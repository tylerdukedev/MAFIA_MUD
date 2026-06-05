#include "character/character_social_network.h"
#include "game/housing_living_costs.h"
#include "game/player_operations.h"
#include "sim/character_agent.h"
#include <catch2/catch_test_macros.hpp>

using namespace Core;

TEST_CASE("Monthly ledger runs after HQ established at tick zero", "[housing_living_costs]") {
    PlayerOperationsStore store{};
    store.headquartersKind = HeadquartersKind::RentedRoom;
    store.headquartersEstablishedTick = 0ULL;
    const uint64_t inputTickCount = static_cast<uint64_t>(MONTHLY_LEDGER_INTERVAL_TICKS);
    REQUIRE(shouldRunMonthlyLedger(store, inputTickCount));
}

TEST_CASE("Monthly rented room ledger includes rent and city fees", "[housing_living_costs]") {
    PlayerOperationsStore store{};
    store.headquartersKind = HeadquartersKind::RentedRoom;
    MonthlyHousingLedger ledger{};
    buildMonthlyHousingLedger(store, -1, ledger);
    REQUIRE(ledger.rentCents == 1400);
    REQUIRE(ledger.taxesAndFeesCents == 250);
    REQUIRE(ledger.totalExpenseCents == 1650);
}

TEST_CASE("Monthly apartment ledger includes utilities", "[housing_living_costs]") {
    PlayerOperationsStore store{};
    store.headquartersKind = HeadquartersKind::Apartment;
    MonthlyHousingLedger ledger{};
    buildMonthlyHousingLedger(store, -1, ledger);
    REQUIRE(ledger.utilitiesCents > 0);
    REQUIRE(ledger.totalExpenseCents > ledger.rentCents);
}

TEST_CASE("Family upkeep restores opinion on cooldown", "[housing_living_costs]") {
    PlayerOperationsStore store{};
    store.headquartersKind = HeadquartersKind::FamilyFriendDpa;
    store.headquartersEstablishedTick = 1ULL;
    PlayerWallet wallet{};
    wallet.cashCents = 5000;
    CharacterAgentStore agents{};
    initializeCharacterAgentStore(agents);
    CharacterDraft draft{};
    draft.hasFamilyInCountry = true;
    draft.characterRollSeed = 77ULL;
    spawnPersonalContactsFromDraft(draft, agents);
    const int32_t beforeOpinion = agents.states[FAMILY_AGENT_SLOT_INDEX].opinionOfPlayer;
    REQUIRE(tryApplyFamilyUpkeep(store, wallet, agents, 0, 200ULL));
    REQUIRE(agents.states[FAMILY_AGENT_SLOT_INDEX].opinionOfPlayer > beforeOpinion);
}
