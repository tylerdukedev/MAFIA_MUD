#include "character/character_family.h"
#include "character/character_social_network.h"
#include "character/profile_builder.h"
#include "game/housing_living_costs.h"
#include "game/kin_housing.h"
#include "game/player_employment.h"
#include "game/player_operations.h"
#include "sim/character_agent.h"
#include "world/business_node_table.h"
#include <catch2/catch_test_macros.hpp>
#include <climits>
#include <cstring>

using namespace Core;

namespace {

constexpr int32_t JOB_INTERVIEW_BEST_SCORE = 8;
constexpr int32_t JOB_INTERVIEW_WORST_SCORE = -2;
constexpr int32_t JOB_INTERVIEW_MIXED_PASS_SCORE = 5;

int32_t findLowestWageBusinessIndex() {
    int32_t lowestIndex = 0;
    int64_t lowestWage = INT64_MAX;
    const int32_t businessCount = getBusinessNodeCount();
    for (int32_t businessIndex = 0; businessIndex < businessCount; ++businessIndex) {
        const BusinessNodeDefinition* business = getBusinessNodeDefinition(businessIndex);
        if (business == nullptr) {
            continue;
        }
        if (business->jobWageCents < lowestWage) {
            lowestWage = business->jobWageCents;
            lowestIndex = businessIndex;
        }
    }
    return lowestIndex;
}

} // namespace

TEST_CASE("Unemployed player earns zero legit income per tick", "[game_integration][economy]") {
    PlayerOperationsStore store{};
    REQUIRE(computeEmployedLegitIncomePerTickCents(store) == 0.0f);
}

TEST_CASE("Employed income matches monthly ledger wage and covers rented room", "[game_integration][economy]") {
    const int32_t lowestWageIndex = findLowestWageBusinessIndex();
    const BusinessNodeDefinition* business = getBusinessNodeDefinition(lowestWageIndex);
    REQUIRE(business != nullptr);
    PlayerOperationsStore store{};
    store.employedBusinessIndex = lowestWageIndex;
    const float perTickIncome = computeEmployedLegitIncomePerTickCents(store);
    const int64_t expectedMonthlyIncome = business->jobWageCents * JOB_MONTHLY_WAGE_MULTIPLIER;
    const float simulatedMonthlyIncome = perTickIncome * static_cast<float>(MONTHLY_LEDGER_INTERVAL_TICKS);
    REQUIRE(static_cast<int64_t>(simulatedMonthlyIncome) == expectedMonthlyIncome);
    store.headquartersKind = HeadquartersKind::RentedRoom;
    MonthlyHousingLedger ledger{};
    buildMonthlyHousingLedger(store, lowestWageIndex, ledger);
    REQUIRE(ledger.jobIncomeCents == expectedMonthlyIncome);
    REQUIRE(ledger.netCashDeltaCents > 0);
    REQUIRE(ledger.jobIncomeCents > ledger.totalExpenseCents);
}

TEST_CASE("Job interview pass threshold separates strong and weak answers", "[game_integration][employment]") {
    PlayerOperationsStore store{};
    PlayerProfile profile = buildPlayerProfile(CharacterDraft{});
    const int32_t businessIndex = 0;
    REQUIRE_FALSE(tryHirePlayerAtBusiness(store, profile, businessIndex, JOB_INTERVIEW_WORST_SCORE));
    REQUIRE_FALSE(tryHirePlayerAtBusiness(store, profile, businessIndex, JOB_INTERVIEW_PASS_SCORE - 1));
    REQUIRE(tryHirePlayerAtBusiness(store, profile, businessIndex, JOB_INTERVIEW_PASS_SCORE));
    store.employedBusinessIndex = -1;
    REQUIRE(tryHirePlayerAtBusiness(store, profile, businessIndex, JOB_INTERVIEW_BEST_SCORE));
    REQUIRE(store.employedBusinessIndex == businessIndex);
}

TEST_CASE("Italian family cultural profile raises kin loyalty versus neutral defaults", "[game_integration][family]") {
    CharacterDraft italianDraft{};
    italianDraft.heritageId = HeritageId::Italian;
    italianDraft.nationalityId = NationalityId::American;
    italianDraft.generationId = GenerationId::FirstGeneration;
    italianDraft.characterRollSeed = 4242ULL;
    rollCharacterSocialNetwork(italianDraft);
    generatePlayerFamilyTree(italianDraft);
    const PlayerProfile italianProfile = buildPlayerProfile(italianDraft);
    CharacterDraft neutralDraft = italianDraft;
    neutralDraft.familyCulturalProfile = FamilyCulturalProfile{};
    const PlayerProfile neutralProfile = buildPlayerProfile(neutralDraft);
    REQUIRE(italianDraft.familyCulturalProfile.kinLoyaltyPressure > neutralDraft.familyCulturalProfile.kinLoyaltyPressure);
    REQUIRE(italianProfile.loyaltyBias.kinAlliancePreference > neutralProfile.loyaltyBias.kinAlliancePreference);
}

TEST_CASE("Family tree names avoid placeholder characters and abroad when kin not in-country", "[game_integration][family]") {
    CharacterDraft draft{};
    draft.heritageId = HeritageId::Italian;
    draft.nationalityId = NationalityId::American;
    draft.generationId = GenerationId::Immigrant;
    draft.characterRollSeed = 9001ULL;
    draft.hasFamilyInCountry = false;
    generatePlayerFamilyTree(draft);
    REQUIRE(draft.familyMemberCount >= 2);
    for (int32_t index = 0; index < draft.familyMemberCount; ++index) {
        const FamilyMemberRecord& member = draft.familyMembers[index];
        REQUIRE(std::strlen(member.displayName) > 0);
        REQUIRE(std::strchr(member.displayName, '?') == nullptr);
        REQUIRE(member.presence == FamilyMemberPresence::Abroad);
        REQUIRE_FALSE(member.isInCountry);
    }
}

TEST_CASE("Family presence labels match borough versus city versus abroad", "[game_integration][family]") {
    REQUIRE(std::strcmp(getFamilyMemberPresenceLabel(FamilyMemberPresence::SameBorough), "your borough") == 0);
    REQUIRE(std::strcmp(getFamilyMemberPresenceLabel(FamilyMemberPresence::OtherBoroughInCity), "elsewhere in the city") == 0);
    REQUIRE(std::strcmp(getFamilyMemberPresenceLabel(FamilyMemberPresence::Abroad), "abroad") == 0);
}

TEST_CASE("Kin DPA headquarters promotes family contact to landlord slot", "[game_integration][housing]") {
    CharacterDraft draft{};
    draft.hasFamilyInCountry = true;
    draft.hasFriendsInCountry = false;
    draft.characterRollSeed = 77ULL;
    CharacterAgentStore agents{};
    initializeCharacterAgentStore(agents);
    spawnPersonalContactsFromDraft(draft, agents);
    const PlayerProfile profile = buildPlayerProfile(draft);
    PlayerOperationsStore store{};
    PlayerWallet wallet{};
    wallet.cashCents = 500;
    REQUIRE(tryEstablishFamilyFriendDpaHeadquarters(store, wallet, profile, agents, 1ULL));
    REQUIRE(store.headquartersKind == HeadquartersKind::FamilyFriendDpa);
    REQUIRE(agents.states[FIRST_COMMUNITY_AGENT_SLOT_INDEX].isActive);
    REQUIRE(std::strstr(agents.states[FIRST_COMMUNITY_AGENT_SLOT_INDEX].generatedRoleLabel, "Landlord") != nullptr);
}

TEST_CASE("Friend-only kin can open DPA when family is not in-country", "[game_integration][housing]") {
    CharacterDraft draft{};
    draft.hasFamilyInCountry = false;
    draft.hasFriendsInCountry = true;
    draft.characterRollSeed = 88ULL;
    CharacterAgentStore agents{};
    initializeCharacterAgentStore(agents);
    spawnPersonalContactsFromDraft(draft, agents);
    const PlayerProfile profile = buildPlayerProfile(draft);
    PlayerOperationsStore store{};
    PlayerWallet wallet{};
    REQUIRE(tryEstablishFamilyFriendDpaHeadquarters(store, wallet, profile, agents, 1ULL));
    REQUIRE(store.headquartersKind == HeadquartersKind::FamilyFriendDpa);
    REQUIRE(agents.states[FRIEND_AGENT_SLOT_INDEX].isActive);
}

TEST_CASE("Family DPA monthly ledger skips rent while rented room still bills", "[game_integration][housing]") {
    PlayerOperationsStore dpaStore{};
    dpaStore.headquartersKind = HeadquartersKind::FamilyFriendDpa;
    MonthlyHousingLedger dpaLedger{};
    buildMonthlyHousingLedger(dpaStore, 0, dpaLedger);
    REQUIRE(dpaLedger.rentCents == 0);
    REQUIRE(dpaLedger.totalExpenseCents == 0);
    PlayerOperationsStore roomStore{};
    roomStore.headquartersKind = HeadquartersKind::RentedRoom;
    MonthlyHousingLedger roomLedger{};
    buildMonthlyHousingLedger(roomStore, -1, roomLedger);
    REQUIRE(roomLedger.totalExpenseCents > 0);
}

TEST_CASE("Starting contact preview includes rolled personal agents", "[game_integration][contacts]") {
    CharacterDraft draft{};
    draft.hasFamilyInCountry = true;
    draft.hasFriendsInCountry = true;
    draft.characterRollSeed = 99123ULL;
    CharacterAgentStore store{};
    buildStartingContactPreviewStore(draft, store);
    REQUIRE(store.states[FAMILY_AGENT_SLOT_INDEX].isActive);
    REQUIRE(store.states[FRIEND_AGENT_SLOT_INDEX].isActive);
}

TEST_CASE("Interview mixed competent answers meet pass score", "[game_integration][employment]") {
    REQUIRE(JOB_INTERVIEW_MIXED_PASS_SCORE >= JOB_INTERVIEW_PASS_SCORE);
    REQUIRE(JOB_INTERVIEW_BEST_SCORE > JOB_INTERVIEW_MIXED_PASS_SCORE);
}
