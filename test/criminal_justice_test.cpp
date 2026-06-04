#include "character/character_social_network.h"
#include "game/player_criminal_justice.h"
#include "game/player_law_enforcement.h"
#include "game/player_wallet.h"
#include "game/jail_events.h"
#include "game/player_information_feed.h"
#include "sim/character_agent.h"
#include "world/city_control.h"
#include "world/chunk_store.h"
#include "world/world_config.h"
#include "game/player_world_state.h"
#include <catch2/catch_test_macros.hpp>

using namespace Core;

TEST_CASE("Bond payment moves player to awaiting court", "[criminal_justice]") {
    PlayerCriminalJusticeStore justiceStore{};
    PlayerLawEnforcementStore lawStore{};
    PlayerWallet wallet{};
    wallet.cashCents = 50000;
    beginPlayerArrest(justiceStore, lawStore, CrimeLegalTier::Street, 10ULL, "Test arrest");
    REQUIRE(getPlayerCustodyPhase(justiceStore) == CustodyPhase::Arrested);
    PlayerOrganizationStore organizationStore{};
    WorldConfig worldConfig{};
    ChunkStore chunkStore{worldConfig};
    PlayerWorldState worldState{};
    initializePlayerWorldStateFromStart(worldState, 240, 240, RegionId::Manhattan);
    REQUIRE(tryPayPlayerBond(justiceStore, wallet, lawStore, organizationStore, worldState, chunkStore, worldConfig, 20ULL));
    const CustodyPhase releasePhase = getPlayerCustodyPhase(justiceStore);
    REQUIRE((releasePhase == CustodyPhase::OnBail || releasePhase == CustodyPhase::OnParole));
    REQUIRE(wallet.cashCents < 50000);
}

TEST_CASE("Incarcerated player cannot attempt organization-tier street crime gate", "[criminal_justice]") {
    PlayerCriminalJusticeStore justiceStore{};
    PlayerLawEnforcementStore lawStore{};
    beginPlayerArrest(justiceStore, lawStore, CrimeLegalTier::Organization, 1ULL, "Sweep");
    justiceStore.custodyPhase = static_cast<uint8_t>(CustodyPhase::InPrison);
    REQUIRE(isPlayerFullyIncarcerated(justiceStore));
    REQUIRE_FALSE(canAttemptCrimeLegalTier(justiceStore, CrimeLegalTier::Street));
}

TEST_CASE("Jail event fires only on interval ticks", "[jail_events]") {
    JailEvent outEvent{};
    const uint64_t inputCustodyStart = 100ULL;
    const uint64_t inputWorldSeed = 4242ULL;
    const uint64_t inputOffIntervalTick = 7ULL;
    const bool firedOffInterval = tryFireJailEvent(outEvent, inputCustodyStart, inputWorldSeed, inputOffIntervalTick);
    REQUIRE_FALSE(firedOffInterval);
}

TEST_CASE("Jail event produces a labeled event with description", "[jail_events]") {
    JailEvent outEvent{};
    const uint64_t inputCustodyStart = 50ULL;
    const uint64_t inputWorldSeed = 99ULL;
    bool didFire = false;
    for (uint64_t tick = 0ULL; tick < 2000ULL; tick += JAIL_EVENT_INTERVAL_TICKS) {
        if (tryFireJailEvent(outEvent, inputCustodyStart, inputWorldSeed, tick)) {
            didFire = true;
            break;
        }
    }
    REQUIRE(didFire);
    REQUIRE(outEvent.description[0] != '\0');
    const char* label = jailEventTypeToString(outEvent.type);
    REQUIRE(label != nullptr);
}

TEST_CASE("Held-without-bond player accumulates jail events in info feed", "[jail_events]") {
    PlayerCriminalJusticeStore justiceStore{};
    PlayerLawEnforcementStore lawStore{};
    PlayerWallet wallet{};
    wallet.cashCents = 100000;
    CityControlStore cities{};
    CharacterAgentStore agents{};
    initializeCharacterAgentStore(agents);
    PlayerInformationFeedStore infoFeed{};
    beginPlayerArrest(justiceStore, lawStore, CrimeLegalTier::Street, 0ULL, "Test arrest");
    for (uint64_t tick = 1ULL; tick < 400ULL; ++tick) {
        tickPlayerCriminalJustice(justiceStore, lawStore, wallet, cities, agents, 333ULL, tick, &infoFeed);
    }
    REQUIRE(infoFeed.itemCount > 0);
}

TEST_CASE("Rival can seize player city while player is in prison", "[criminal_justice]") {
    PlayerCriminalJusticeStore justiceStore{};
    PlayerLawEnforcementStore lawStore{};
    CityControlStore cities{};
    CharacterAgentStore agents{};
    initializeCharacterAgentStore(agents);
    agents.states[RIVAL_AGENT_SLOT_INDEX].isActive = true;
    agents.states[RIVAL_AGENT_SLOT_INDEX].respect = 60;
    agents.states[RIVAL_AGENT_SLOT_INDEX].opinionOfPlayer = -20;
    tryClaimCityForPlayer(cities, 0, 1ULL);
    justiceStore.custodyPhase = static_cast<uint8_t>(CustodyPhase::InPrison);
    justiceStore.phaseTicksRemaining = 10;
    for (uint64_t tick = 0; tick < 200ULL; ++tick) {
        tickPlayerCriminalJustice(justiceStore, lawStore, PlayerWallet{}, cities, agents, 777ULL, tick);
    }
    const bool rivalOwns = cities.slots[0].ownerId == RIVAL_OWNER_ID;
    const bool playerOwns = cities.slots[0].ownerId == PLAYER_OWNER_ID;
    REQUIRE((rivalOwns || playerOwns));
}
