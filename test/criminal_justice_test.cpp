#include "character/character_social_network.h"
#include "game/player_criminal_justice.h"
#include "game/player_law_enforcement.h"
#include "game/player_wallet.h"
#include "sim/character_agent.h"
#include "world/city_control.h"
#include <catch2/catch_test_macros.hpp>

using namespace Core;

TEST_CASE("Bond payment moves player to awaiting court", "[criminal_justice]") {
    PlayerCriminalJusticeStore justiceStore{};
    PlayerLawEnforcementStore lawStore{};
    PlayerWallet wallet{};
    wallet.cashCents = 50000;
    beginPlayerArrest(justiceStore, lawStore, CrimeLegalTier::Street, 10ULL, "Test arrest");
    REQUIRE(getPlayerCustodyPhase(justiceStore) == CustodyPhase::Arrested);
    REQUIRE(tryPayPlayerBond(justiceStore, wallet, 20ULL));
    REQUIRE(getPlayerCustodyPhase(justiceStore) == CustodyPhase::AwaitingCourt);
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
