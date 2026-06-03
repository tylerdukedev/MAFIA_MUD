#include "character/profile_builder.h"
#include "game/player_law_enforcement.h"
#include "game/player_organization.h"
#include "game/player_operations.h"
#include "game/player_wallet.h"
#include "game/street_crime.h"
#include "sim/character_agent.h"
#include <catch2/catch_test_macros.hpp>

using namespace Core;

TEST_CASE("Solo street crime pays cash when headquarters is set", "[street_crime]") {
    CharacterDraft draft{};
    draft.characterRollSeed = 42ULL;
    const PlayerProfile profile = buildPlayerProfile(draft);
    PlayerOperationsStore operations{};
    operations.headquartersKind = HeadquartersKind::RentedRoom;
    PlayerStreetCrimeStore crimeStore{};
    PlayerLawEnforcementStore lawStore{};
    PlayerOrganizationStore organization{};
    PlayerWallet wallet{};
    CharacterAgentStore agents{};
    initializeCharacterAgentStore(agents);
    const bool committed = tryCommitStreetCrime(operations, crimeStore, lawStore, organization, wallet, agents, profile, 0, 100ULL, 999ULL);
    if (committed) {
        REQUIRE(wallet.cashCents > 0);
        REQUIRE(lawStore.personalHeat > 0);
    }
}

TEST_CASE("Crew street crime stays locked without trusted criminal contact", "[street_crime]") {
    CharacterDraft draft{};
    const PlayerProfile profile = buildPlayerProfile(draft);
    PlayerOperationsStore operations{};
    operations.headquartersKind = HeadquartersKind::RentedRoom;
    PlayerStreetCrimeStore crimeStore{};
    PlayerLawEnforcementStore lawStore{};
    PlayerOrganizationStore organization{};
    CharacterAgentStore agents{};
    initializeCharacterAgentStore(agents);
    const StreetCrimeDefinition* crewCrime = getStreetCrimeDefinition(3);
    REQUIRE(crewCrime != nullptr);
    REQUIRE(crewCrime->tier == StreetCrimeTier::Crew);
    const StreetCrimeLockReason lockReason = evaluateStreetCrimeLock(
        operations, crimeStore, lawStore, organization, profile, agents, 3, *crewCrime, 200ULL);
    REQUIRE(lockReason == StreetCrimeLockReason::NeedsCrewTier);
}
