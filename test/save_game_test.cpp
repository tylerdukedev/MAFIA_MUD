#include "persistence/save_game.h"
#include "persistence/save_gameplay_stores.h"
#include "game/player_law_enforcement.h"
#include "game/player_organization.h"
#include "game/street_crime.h"
#include "procgen/world_generator.h"
#include "world/tile_vitality.h"
#include "world/city_control.h"
#include "game/player_operations.h"
#include "game/player_wallet.h"
#include "character/character_social_network.h"
#include "sim/character_agent.h"
#include "sim/world_event_store.h"
#include "game/bank_loan.h"
#include "game/evidence_system.h"
#include "game/investigation_case_store.h"
#include "game/property_listing_store.h"
#include "game/property_store.h"
#include "game/shared_travel_state.h"
#include "character/character_social_network.h"
#include "character/profile_builder.h"
#include <catch2/catch_test_macros.hpp>
#include <cstdio>
#include <string>

using namespace Core;

namespace {
constexpr const char* TEST_SAVE_PATH = "capitalvice_test_save.dat";

void removeTestSaveFile() {
    std::remove(TEST_SAVE_PATH);
}
} // namespace

TEST_CASE("SaveGame round-trip preserves world state", "[persistence]") {
    removeTestSaveFile();
    WorldConfig config;
    WorldGenerator generator;
    ChunkStore sourceStore(config);
    generator.generate(config, sourceStore, DEFAULT_WORLD_SEED);
    CharacterDraft sourceDraft{};
    std::snprintf(sourceDraft.nameBuffer, sizeof(sourceDraft.nameBuffer), "%s", "Test Boss");
    sourceDraft.nationalityId = NationalityId::American;
    sourceDraft.heritageId = HeritageId::Italian;
    sourceDraft.generationId = GenerationId::FirstGeneration;
    sourceDraft.age = 22;
    sourceDraft.backgroundId = BackgroundId::StreetHustler;
    sourceDraft.selectedBoroughIndex = 1;
    sourceDraft.hasInitializedDefaults = true;
    SimClock sourceClock(WorldConfig::DEFAULT_TICK_RATE_HZ);
    sourceClock.restoreSnapshot(42U, false, 2.0, 0.05);
    MapCamera sourceCamera{};
    sourceCamera.centerWorldX = 120.0f;
    sourceCamera.centerWorldY = 88.0f;
    sourceCamera.pixelsPerTile = 3.5f;
    BoroughVitalityStore sourceVitality{};
    rollupBoroughVitality(config, sourceStore, sourceVitality);
    PlayerWallet sourceWallet{};
    sourceWallet.cashCents = 1250;
    sourceWallet.lifetimeLegitCents = 300;
    sourceWallet.lifetimeCrimeCents = 950;
    CityControlStore sourceCities{};
    resetCityControlStore(sourceCities);
    REQUIRE(tryClaimCityForPlayer(sourceCities, 3, 7U));
    PlayerOperationsStore sourceOperations{};
    sourceOperations.headquartersKind = HeadquartersKind::RentedRoom;
    sourceOperations.headquartersEstablishedTick = 100ULL;
    sourceOperations.lastMonthlyLedgerTick = 900ULL;
    constexpr int32_t REAPPLY_BUSINESS_INDEX = 5;
    constexpr uint64_t REAPPLY_TICK = 7777ULL;
    sourceOperations.jobReapplyAvailableTickByBusiness[REAPPLY_BUSINESS_INDEX] = REAPPLY_TICK;
    sourceOperations.homePropertyIndex = 3;
    sourceOperations.housingTenure = HousingTenure::Rent;
    CharacterAgentStore sourceAgents{};
    initializeCharacterAgentStore(sourceAgents);
    PropertyListingStore sourceListings{};
    const int32_t listingIndex = addPropertyListing(
        sourceListings,
        40,
        50,
        2U,
        PropertyListingTier::Apartment,
        static_cast<uint8_t>(PropertyListingPerks::NearTransit),
        25000000LL,
        180000LL,
        2U,
        1);
    REQUIRE(listingIndex >= 0);
    BankLoanStore sourceLoans{};
    sourceLoans.loans[0].kind = LoanKind::Mortgage;
    sourceLoans.loans[0].principalRemainingCents = 18000000LL;
    sourceLoans.loans[0].aprBps = 650;
    sourceLoans.loans[0].termMonthsRemaining = 240;
    sourceLoans.loans[0].monthlyPaymentCents = 125000LL;
    sourceLoans.loans[0].isActive = true;
    sourceLoans.loans[0].regionId = 2U;
    sourceLoans.activeLoanCount = 1;
    sourceLoans.lastMonthlyPaymentTick = 800ULL;
    InvestigationCaseStore sourceInvestigations{};
    const int32_t caseIndex = openInvestigationCase(
        sourceInvestigations,
        CrimeLegalTier::Street,
        900ULL,
        "Test case");
    REQUIRE(caseIndex >= 0);
    EvidenceSystemStore sourceEvidence{};
    REQUIRE(tryAddEvidenceToCase(
        sourceEvidence,
        sourceInvestigations,
        caseIndex,
        EvidenceKind::Witness,
        EVIDENCE_WEIGHT_WITNESS,
        "Witness saw suspect",
        901ULL));
    sourceDraft.hasFamilyInCountry = true;
    sourceDraft.characterRollSeed = 4242ULL;
    spawnPersonalContactsFromDraft(sourceDraft, sourceAgents);
    sourceAgents.states[FAMILY_AGENT_SLOT_INDEX].mobilityAsset = MobilityAsset::Bicycle;
    sourceAgents.states[FAMILY_AGENT_SLOT_INDEX].travelPathTileCount = 2;
    sourceAgents.states[FAMILY_AGENT_SLOT_INDEX].travelPathTileX[0] = 10;
    sourceAgents.states[FAMILY_AGENT_SLOT_INDEX].travelPathTileY[0] = 20;
    sourceAgents.states[FAMILY_AGENT_SLOT_INDEX].travelPathTileX[1] = 11;
    sourceAgents.states[FAMILY_AGENT_SLOT_INDEX].travelPathTileY[1] = 21;
    sourceAgents.states[FAMILY_AGENT_SLOT_INDEX].travelStartTick = 50ULL;
    sourceAgents.states[FAMILY_AGENT_SLOT_INDEX].travelCompleteTick = 120ULL;
    sourceAgents.states[FAMILY_AGENT_SLOT_INDEX].travelMode = TravelMode::Bicycle;
    sourceAgents.states[FAMILY_AGENT_SLOT_INDEX].travelDestTileX = 11;
    sourceAgents.states[FAMILY_AGENT_SLOT_INDEX].travelDestTileY = 21;
    SaveGameSnapshot snapshot{};
    REQUIRE(buildSaveSnapshot(
        snapshot,
        DEFAULT_WORLD_SEED,
        sourceDraft,
        sourceClock,
        sourceCamera,
        sourceStore,
        sourceVitality,
        sourceWallet,
        sourceCities,
        sourceOperations,
        WorldEventStore{},
        sourceAgents,
        PropertyStore{},
        sourceListings,
        sourceLoans,
        sourceInvestigations,
        sourceEvidence,
        PlayerOrganizationStore{},
        PlayerLawEnforcementStore{},
        PlayerStreetCrimeStore{},
        PlayerCriminalJusticeStore{},
        CriminalRecordStore{},
        PoliceContactStore{},
        SaveGameplayStores{},
        0));
    REQUIRE(saveGameToFile(TEST_SAVE_PATH, snapshot));
    REQUIRE(saveFileExists(TEST_SAVE_PATH));
    SaveGameSnapshot loadedSnapshot{};
    REQUIRE(loadGameFromFile(TEST_SAVE_PATH, loadedSnapshot));
    ChunkStore loadedStore(config);
    SimClock loadedClock(WorldConfig::DEFAULT_TICK_RATE_HZ);
    MapCamera loadedCamera{};
    CharacterDraft loadedDraft{};
    PlayerWallet loadedWallet{};
    CityControlStore loadedCities{};
    PlayerOperationsStore loadedOperations{};
    CharacterAgentStore loadedAgents{};
    PropertyStore loadedPropertyStore{};
    PropertyListingStore loadedListings{};
    BankLoanStore loadedLoans{};
    InvestigationCaseStore loadedInvestigations{};
    EvidenceSystemStore loadedEvidence{};
    WorldEventStore loadedWorldEvents{};
    PlayerOrganizationStore loadedOrganization{};
    PlayerLawEnforcementStore loadedLaw{};
    PlayerStreetCrimeStore loadedStreetCrime{};
    PlayerCriminalJusticeStore loadedJustice{};
    CriminalRecordStore loadedCriminalRecord{};
    PoliceContactStore loadedPoliceContacts{};
    uint64_t loadedSeed = 0;
    REQUIRE(applySaveSnapshot(
        loadedSnapshot,
        loadedSeed,
        loadedDraft,
        loadedClock,
        loadedCamera,
        loadedStore,
        loadedWallet,
        loadedCities,
        loadedOperations,
        loadedWorldEvents,
        loadedAgents,
        loadedPropertyStore,
        loadedListings,
        loadedLoans,
        loadedInvestigations,
        loadedEvidence,
        loadedOrganization,
        loadedLaw,
        loadedStreetCrime,
        loadedJustice,
        loadedCriminalRecord,
        loadedPoliceContacts,
        SaveGameplayStores{},
        loadedOperations.workExperienceMonths));
    const WorldCoord sampleCoord{200, 180};
    REQUIRE(loadedSeed == DEFAULT_WORLD_SEED);
    REQUIRE(loadedDraft.heritageId == sourceDraft.heritageId);
    REQUIRE(loadedDraft.generationId == sourceDraft.generationId);
    REQUIRE(loadedDraft.age == sourceDraft.age);
    REQUIRE(std::string(loadedDraft.nameBuffer) == std::string(sourceDraft.nameBuffer));
    REQUIRE(loadedClock.getTickCount() == sourceClock.getTickCount());
    REQUIRE(loadedClock.isPaused() == sourceClock.isPaused());
    REQUIRE(loadedStore.getTerrainAt(sampleCoord) == sourceStore.getTerrainAt(sampleCoord));
    REQUIRE(loadedStore.getRegionAt(sampleCoord) == sourceStore.getRegionAt(sampleCoord));
    REQUIRE(loadedStore.getElevationAt(sampleCoord) == sourceStore.getElevationAt(sampleCoord));
    REQUIRE(loadedStore.getEconomicWeightAt(sampleCoord) == sourceStore.getEconomicWeightAt(sampleCoord));
    REQUIRE(loadedStore.getPopulationAt(sampleCoord) == sourceStore.getPopulationAt(sampleCoord));
    REQUIRE(loadedWallet.cashCents == sourceWallet.cashCents);
    REQUIRE(loadedWallet.lifetimeLegitCents == sourceWallet.lifetimeLegitCents);
    REQUIRE(loadedWallet.lifetimeCrimeCents == sourceWallet.lifetimeCrimeCents);
    REQUIRE(getCityOwnerId(loadedCities, 3) == PLAYER_OWNER_ID);
    REQUIRE(loadedOperations.headquartersKind == HeadquartersKind::RentedRoom);
    REQUIRE(loadedOperations.headquartersEstablishedTick == sourceOperations.headquartersEstablishedTick);
    REQUIRE(loadedOperations.lastMonthlyLedgerTick == sourceOperations.lastMonthlyLedgerTick);
    REQUIRE(loadedOperations.jobReapplyAvailableTickByBusiness[REAPPLY_BUSINESS_INDEX] == REAPPLY_TICK);
    REQUIRE(loadedOperations.homePropertyIndex == sourceOperations.homePropertyIndex);
    REQUIRE(loadedOperations.housingTenure == sourceOperations.housingTenure);
    REQUIRE(getCharacterAgentState(loadedAgents, 0) != nullptr);
    const CharacterAgentState* loadedAgent = getCharacterAgentState(loadedAgents, FAMILY_AGENT_SLOT_INDEX);
    REQUIRE(loadedAgent != nullptr);
    REQUIRE(loadedAgent->mobilityAsset == MobilityAsset::Bicycle);
    REQUIRE(loadedAgent->travelPathTileCount == 2);
    REQUIRE(loadedAgent->travelPathTileX[0] == 10);
    REQUIRE(loadedAgent->travelPathTileY[1] == 21);
    REQUIRE(loadedAgent->travelStartTick == 50ULL);
    REQUIRE(loadedAgent->travelCompleteTick == 120ULL);
    REQUIRE(loadedAgent->travelMode == TravelMode::Bicycle);
    REQUIRE(loadedListings.listingCount == sourceListings.listingCount);
    const PropertyListingRecord* loadedListing = getPropertyListingRecord(loadedListings, listingIndex);
    REQUIRE(loadedListing != nullptr);
    REQUIRE(loadedListing->tileX == 40);
    REQUIRE(loadedListing->rentCents == 180000LL);
    REQUIRE(loadedLoans.activeLoanCount == 1);
    REQUIRE(loadedLoans.loans[0].principalRemainingCents == 18000000LL);
    REQUIRE(loadedLoans.lastMonthlyPaymentTick == 800ULL);
    REQUIRE(loadedInvestigations.activeCount == sourceInvestigations.activeCount);
    const InvestigationCase* loadedCase = getInvestigationCase(loadedInvestigations, caseIndex);
    REQUIRE(loadedCase != nullptr);
    REQUIRE(loadedCase->isActive != 0U);
    REQUIRE(computeCaseEvidenceScore(loadedEvidence, caseIndex) == computeCaseEvidenceScore(sourceEvidence, caseIndex));
    removeTestSaveFile();
}

TEST_CASE("SaveGame rejects missing save file", "[persistence]") {
    removeTestSaveFile();
    SaveGameSnapshot snapshot{};
    REQUIRE_FALSE(loadGameFromFile(TEST_SAVE_PATH, snapshot));
    REQUIRE_FALSE(saveFileExists(TEST_SAVE_PATH));
}
