#include "persistence/save_game.h"
#include "persistence/save_gameplay_stores.h"
#include "game/player_information_feed.h"
#include "game/player_narrative_archive.h"
#include "game/player_criminal_justice.h"
#include "game/player_law_enforcement.h"
#include "game/player_organization.h"
#include "game/street_crime.h"
#include "sim/character_agent.h"
#include "sim/world_event_types.h"
#include "world/landmark_table.h"
#include <cstddef>
#include <cstdio>
#include <cstring>

namespace Core {

namespace {
constexpr char SAVE_MAGIC[4] = {'C', 'V', 'S', 'V'};
constexpr uint32_t SAVE_VERSION_MIN = 10U;
constexpr uint32_t SAVE_VERSION = 14U;

struct SaveGameHeader {
    char magic[4];
    uint32_t version;
    uint64_t worldSeed;
    char nameBuffer[32];
    uint8_t nationalityId;
    uint8_t heritageId;
    uint8_t generationId;
    uint8_t backgroundId;
    int32_t age;
    int32_t selectedBoroughIndex;
    int32_t startingCityLandmarkIndex;
    uint8_t hasInitializedDefaults;
    int64_t cashCents;
    int64_t lifetimeLegitCents;
    int64_t lifetimeCrimeCents;
    uint64_t tickCount;
    uint8_t isPaused;
    uint8_t tickPadding[7];
    double speedMultiplier;
    double accumulatorSeconds;
    float centerWorldX;
    float centerWorldY;
    float pixelsPerTile;
    int32_t worldWidthTiles;
    int32_t worldHeightTiles;
    uint8_t headquartersKind;
    uint8_t hasFamilyInCountry;
    uint8_t hasFriendsInCountry;
    uint8_t savePadding;
    int32_t employedBusinessIndices[2];
    int32_t activeOperationCount;
    int32_t familyOpinionPenalty;
    uint64_t headquartersEstablishedTick;
    uint64_t lastMonthlyLedgerTick;
    uint64_t lastFamilyUpkeepTick;
    uint32_t worldEventFlags;
    uint64_t worldEventFiredOnceMask;
    uint8_t headquartersRegionId;
    int8_t consecutiveUnpaidRentMonths;
    int16_t rentMultiplierBps;
    int16_t rentEventAdjustmentBps;
    char worldEventMessage[MAX_WORLD_EVENT_MESSAGE_LENGTH];
};

bool writeAllBytes(FILE* fileHandle, const void* data, size_t byteCount) {
    if (fileHandle == nullptr || data == nullptr) {
        return false;
    }
    const size_t writtenCount = std::fwrite(data, 1, byteCount, fileHandle);
    return writtenCount == byteCount;
}

bool readAllBytes(FILE* fileHandle, void* data, size_t byteCount) {
    if (fileHandle == nullptr || data == nullptr) {
        return false;
    }
    const size_t readCount = std::fread(data, 1, byteCount, fileHandle);
    return readCount == byteCount;
}
} // namespace

bool saveFileExists(const char* filePath) {
    if (filePath == nullptr) {
        return false;
    }
    FILE* fileHandle = std::fopen(filePath, "rb");
    if (fileHandle == nullptr) {
        return false;
    }
    std::fclose(fileHandle);
    return true;
}

bool buildSaveSnapshot(
    SaveGameSnapshot& outSnapshot,
    uint64_t worldSeed,
    const CharacterDraft& characterDraft,
    const SimClock& simClock,
    const MapCamera& mapCamera,
    const ChunkStore& chunkStore,
    const BoroughVitalityStore& boroughVitalityStore,
    const PlayerWallet& playerWallet,
    const CityControlStore& cityControlStore,
    const PlayerOperationsStore& playerOperationsStore,
    const WorldEventStore& worldEventStore,
    const CharacterAgentStore& characterAgentStore,
    const PlayerOrganizationStore& organizationStore,
    const PlayerLawEnforcementStore& lawEnforcementStore,
    const PlayerStreetCrimeStore& streetCrimeStore,
    const PlayerCriminalJusticeStore& criminalJusticeStore,
    const SaveGameplayStores& gameplayStores,
    int32_t workExperienceMonths) {
    (void)boroughVitalityStore;
    outSnapshot.worldSeed = worldSeed;
    outSnapshot.characterDraft = characterDraft;
    outSnapshot.cashCents = playerWallet.cashCents;
    outSnapshot.lifetimeLegitCents = playerWallet.lifetimeLegitCents;
    outSnapshot.lifetimeCrimeCents = playerWallet.lifetimeCrimeCents;
    outSnapshot.cityOwnerIds.assign(static_cast<size_t>(getLandmarkCount()), 0U);
    for (int32_t landmarkIndex = 0; landmarkIndex < getLandmarkCount(); ++landmarkIndex) {
        outSnapshot.cityOwnerIds[static_cast<size_t>(landmarkIndex)] = cityControlStore.slots[landmarkIndex].ownerId;
    }
    outSnapshot.headquartersKind = playerOperationsStore.headquartersKind;
    outSnapshot.employedBusinessIndices[0] = playerOperationsStore.employedBusinessIndices[0];
    outSnapshot.employedBusinessIndices[1] = playerOperationsStore.employedBusinessIndices[1];
    outSnapshot.activeOperationCount = playerOperationsStore.activeOperationCount;
    outSnapshot.familyOpinionPenalty = playerOperationsStore.familyOpinionPenalty;
    outSnapshot.headquartersEstablishedTick = playerOperationsStore.headquartersEstablishedTick;
    outSnapshot.lastMonthlyLedgerTick = playerOperationsStore.lastMonthlyLedgerTick;
    outSnapshot.lastFamilyUpkeepTick = playerOperationsStore.lastFamilyUpkeepTick;
    outSnapshot.headquartersRegionId = playerOperationsStore.headquartersRegionId;
    outSnapshot.consecutiveUnpaidRentMonths = playerOperationsStore.consecutiveUnpaidRentMonths;
    outSnapshot.rentMultiplierBps = playerOperationsStore.rentMultiplierBps;
    outSnapshot.rentEventAdjustmentBps = playerOperationsStore.rentEventAdjustmentBps;
    outSnapshot.worldEventStore = worldEventStore;
    for (int32_t index = 0; index < MAX_OPERATION_CATALOG_COUNT; ++index) {
        outSnapshot.activeCatalogIndices[index] = playerOperationsStore.activeCatalogIndices[index];
    }
    outSnapshot.characterAgentStore = characterAgentStore;
    outSnapshot.organizationStore = organizationStore;
    outSnapshot.lawEnforcementStore = lawEnforcementStore;
    outSnapshot.streetCrimeStore = streetCrimeStore;
    outSnapshot.criminalJusticeStore = criminalJusticeStore;
    outSnapshot.workExperienceMonths = playerOperationsStore.workExperienceMonths;
    for (int32_t businessIndex = 0; businessIndex < MAX_BUSINESS_NODE_COUNT; ++businessIndex) {
        outSnapshot.jobReapplyAvailableTickByBusiness[businessIndex] =
            playerOperationsStore.jobReapplyAvailableTickByBusiness[businessIndex];
    }
    outSnapshot.gameplayStores = gameplayStores;
    outSnapshot.tickCount = simClock.getTickCount();
    outSnapshot.isPaused = simClock.isPaused();
    outSnapshot.speedMultiplier = simClock.getSpeedMultiplier();
    outSnapshot.accumulatorSeconds = simClock.getAccumulatorSeconds();
    outSnapshot.mapCamera = mapCamera;
    outSnapshot.regionIds.assign(static_cast<size_t>(SAVE_GAME_TILE_COUNT), 0U);
    outSnapshot.terrainIds.assign(static_cast<size_t>(SAVE_GAME_TILE_COUNT), 0U);
    outSnapshot.elevations.assign(static_cast<size_t>(SAVE_GAME_TILE_COUNT), 0);
    outSnapshot.flags.assign(static_cast<size_t>(SAVE_GAME_TILE_COUNT), 0U);
    outSnapshot.economicWeights.assign(static_cast<size_t>(SAVE_GAME_TILE_COUNT), 0U);
    outSnapshot.populations.assign(static_cast<size_t>(SAVE_GAME_TILE_COUNT), 0U);
    outSnapshot.crimePressures.assign(static_cast<size_t>(SAVE_GAME_TILE_COUNT), 0U);
    outSnapshot.lawPressures.assign(static_cast<size_t>(SAVE_GAME_TILE_COUNT), 0U);
    outSnapshot.businessVitalities.assign(static_cast<size_t>(SAVE_GAME_TILE_COUNT), 0U);
    outSnapshot.playerInfluences.assign(static_cast<size_t>(SAVE_GAME_TILE_COUNT), 0U);
    outSnapshot.oppositionInfluences.assign(static_cast<size_t>(SAVE_GAME_TILE_COUNT), 0U);
    if (!chunkStore.exportFullWorldTiles(
            outSnapshot.regionIds.data(),
            outSnapshot.terrainIds.data(),
            outSnapshot.elevations.data(),
            outSnapshot.flags.data(),
            SAVE_GAME_TILE_COUNT)) {
        return false;
    }
    return chunkStore.exportFullWorldVitality(
        outSnapshot.economicWeights.data(),
        outSnapshot.populations.data(),
        outSnapshot.crimePressures.data(),
        outSnapshot.lawPressures.data(),
        outSnapshot.businessVitalities.data(),
        outSnapshot.playerInfluences.data(),
        outSnapshot.oppositionInfluences.data(),
        SAVE_GAME_TILE_COUNT);
}

bool applySaveSnapshot(
    const SaveGameSnapshot& snapshot,
    uint64_t& outWorldSeed,
    CharacterDraft& outCharacterDraft,
    SimClock& simClock,
    MapCamera& mapCamera,
    ChunkStore& chunkStore,
    PlayerWallet& playerWallet,
    CityControlStore& cityControlStore,
    PlayerOperationsStore& playerOperationsStore,
    WorldEventStore& worldEventStore,
    CharacterAgentStore& characterAgentStore,
    PlayerOrganizationStore& organizationStore,
    PlayerLawEnforcementStore& lawEnforcementStore,
    PlayerStreetCrimeStore& streetCrimeStore,
    PlayerCriminalJusticeStore& criminalJusticeStore,
    SaveGameplayStores& gameplayStores,
    int32_t& workExperienceMonths) {
    if (static_cast<int32_t>(snapshot.regionIds.size()) != SAVE_GAME_TILE_COUNT
        || static_cast<int32_t>(snapshot.terrainIds.size()) != SAVE_GAME_TILE_COUNT
        || static_cast<int32_t>(snapshot.elevations.size()) != SAVE_GAME_TILE_COUNT
        || static_cast<int32_t>(snapshot.flags.size()) != SAVE_GAME_TILE_COUNT
        || static_cast<int32_t>(snapshot.economicWeights.size()) != SAVE_GAME_TILE_COUNT
        || static_cast<int32_t>(snapshot.populations.size()) != SAVE_GAME_TILE_COUNT
        || static_cast<int32_t>(snapshot.crimePressures.size()) != SAVE_GAME_TILE_COUNT
        || static_cast<int32_t>(snapshot.lawPressures.size()) != SAVE_GAME_TILE_COUNT
        || static_cast<int32_t>(snapshot.businessVitalities.size()) != SAVE_GAME_TILE_COUNT
        || static_cast<int32_t>(snapshot.playerInfluences.size()) != SAVE_GAME_TILE_COUNT
        || static_cast<int32_t>(snapshot.oppositionInfluences.size()) != SAVE_GAME_TILE_COUNT) {
        return false;
    }
    if (!chunkStore.importFullWorldTiles(
            snapshot.regionIds.data(),
            snapshot.terrainIds.data(),
            snapshot.elevations.data(),
            snapshot.flags.data(),
            SAVE_GAME_TILE_COUNT)) {
        return false;
    }
    if (!chunkStore.importFullWorldVitality(
            snapshot.economicWeights.data(),
            snapshot.populations.data(),
            snapshot.crimePressures.data(),
            snapshot.lawPressures.data(),
            snapshot.businessVitalities.data(),
            snapshot.playerInfluences.data(),
            snapshot.oppositionInfluences.data(),
            SAVE_GAME_TILE_COUNT)) {
        return false;
    }
    outWorldSeed = snapshot.worldSeed;
    outCharacterDraft = snapshot.characterDraft;
    simClock.restoreSnapshot(snapshot.tickCount, snapshot.isPaused, snapshot.speedMultiplier, snapshot.accumulatorSeconds);
    mapCamera = snapshot.mapCamera;
    playerWallet.cashCents = snapshot.cashCents;
    playerWallet.lifetimeLegitCents = snapshot.lifetimeLegitCents;
    playerWallet.lifetimeCrimeCents = snapshot.lifetimeCrimeCents;
    resetCityControlStore(cityControlStore);
    const int32_t landmarkCount = getLandmarkCount();
    if (static_cast<int32_t>(snapshot.cityOwnerIds.size()) == landmarkCount) {
        for (int32_t landmarkIndex = 0; landmarkIndex < landmarkCount; ++landmarkIndex) {
            cityControlStore.slots[landmarkIndex].ownerId = snapshot.cityOwnerIds[static_cast<size_t>(landmarkIndex)];
        }
    }
    playerOperationsStore.headquartersKind = snapshot.headquartersKind;
    playerOperationsStore.employedBusinessIndices[0] = snapshot.employedBusinessIndices[0];
    playerOperationsStore.employedBusinessIndices[1] = snapshot.employedBusinessIndices[1];
    playerOperationsStore.activeOperationCount = snapshot.activeOperationCount;
    playerOperationsStore.familyOpinionPenalty = snapshot.familyOpinionPenalty;
    playerOperationsStore.headquartersEstablishedTick = snapshot.headquartersEstablishedTick;
    playerOperationsStore.lastMonthlyLedgerTick = snapshot.lastMonthlyLedgerTick;
    playerOperationsStore.lastFamilyUpkeepTick = snapshot.lastFamilyUpkeepTick;
    playerOperationsStore.headquartersRegionId = snapshot.headquartersRegionId;
    playerOperationsStore.consecutiveUnpaidRentMonths = snapshot.consecutiveUnpaidRentMonths;
    playerOperationsStore.rentMultiplierBps = snapshot.rentMultiplierBps;
    playerOperationsStore.rentEventAdjustmentBps = snapshot.rentEventAdjustmentBps;
    playerOperationsStore.workExperienceMonths = snapshot.workExperienceMonths;
    for (int32_t index = 0; index < MAX_OPERATION_CATALOG_COUNT; ++index) {
        playerOperationsStore.activeCatalogIndices[index] = snapshot.activeCatalogIndices[index];
    }
    worldEventStore = snapshot.worldEventStore;
    characterAgentStore = snapshot.characterAgentStore;
    organizationStore = snapshot.organizationStore;
    lawEnforcementStore = snapshot.lawEnforcementStore;
    streetCrimeStore = snapshot.streetCrimeStore;
    criminalJusticeStore = snapshot.criminalJusticeStore;
    workExperienceMonths = snapshot.workExperienceMonths;
    gameplayStores = snapshot.gameplayStores;
    return true;
}

bool saveGameToFile(const char* filePath, const SaveGameSnapshot& snapshot) {
    if (filePath == nullptr) {
        return false;
    }
    FILE* fileHandle = std::fopen(filePath, "wb");
    if (fileHandle == nullptr) {
        return false;
    }
    SaveGameHeader header{};
    std::memcpy(header.magic, SAVE_MAGIC, sizeof(SAVE_MAGIC));
    header.version = SAVE_VERSION;
    header.worldSeed = snapshot.worldSeed;
    std::memcpy(header.nameBuffer, snapshot.characterDraft.nameBuffer, sizeof(header.nameBuffer));
    header.nationalityId = static_cast<uint8_t>(snapshot.characterDraft.nationalityId);
    header.heritageId = static_cast<uint8_t>(snapshot.characterDraft.heritageId);
    header.generationId = static_cast<uint8_t>(snapshot.characterDraft.generationId);
    header.backgroundId = static_cast<uint8_t>(snapshot.characterDraft.backgroundId);
    header.age = snapshot.characterDraft.age;
    header.selectedBoroughIndex = snapshot.characterDraft.selectedBoroughIndex;
    header.startingCityLandmarkIndex = snapshot.characterDraft.startingCityLandmarkIndex;
    header.hasInitializedDefaults = snapshot.characterDraft.hasInitializedDefaults ? 1U : 0U;
    header.cashCents = snapshot.cashCents;
    header.lifetimeLegitCents = snapshot.lifetimeLegitCents;
    header.lifetimeCrimeCents = snapshot.lifetimeCrimeCents;
    header.tickCount = snapshot.tickCount;
    header.isPaused = snapshot.isPaused ? 1U : 0U;
    header.speedMultiplier = snapshot.speedMultiplier;
    header.accumulatorSeconds = snapshot.accumulatorSeconds;
    header.centerWorldX = snapshot.mapCamera.centerWorldX;
    header.centerWorldY = snapshot.mapCamera.centerWorldY;
    header.pixelsPerTile = snapshot.mapCamera.pixelsPerTile;
    header.worldWidthTiles = WorldConfig::WORLD_WIDTH_TILES;
    header.worldHeightTiles = WorldConfig::WORLD_HEIGHT_TILES;
    header.headquartersKind = static_cast<uint8_t>(snapshot.headquartersKind);
    header.hasFamilyInCountry = snapshot.characterDraft.hasFamilyInCountry ? 1U : 0U;
    header.hasFriendsInCountry = snapshot.characterDraft.hasFriendsInCountry ? 1U : 0U;
    header.savePadding = 0U;
    header.employedBusinessIndices[0] = snapshot.employedBusinessIndices[0];
    header.employedBusinessIndices[1] = snapshot.employedBusinessIndices[1];
    header.activeOperationCount = snapshot.activeOperationCount;
    header.familyOpinionPenalty = snapshot.familyOpinionPenalty;
    header.headquartersEstablishedTick = snapshot.headquartersEstablishedTick;
    header.lastMonthlyLedgerTick = snapshot.lastMonthlyLedgerTick;
    header.lastFamilyUpkeepTick = snapshot.lastFamilyUpkeepTick;
    header.worldEventFlags = snapshot.worldEventStore.worldFlags;
    header.worldEventFiredOnceMask = snapshot.worldEventStore.firedOnceMask;
    header.headquartersRegionId = snapshot.headquartersRegionId;
    header.consecutiveUnpaidRentMonths = snapshot.consecutiveUnpaidRentMonths;
    header.rentMultiplierBps = static_cast<int16_t>(snapshot.rentMultiplierBps);
    header.rentEventAdjustmentBps = static_cast<int16_t>(snapshot.rentEventAdjustmentBps);
    std::strncpy(header.worldEventMessage, snapshot.worldEventStore.lastPlayerMessage, MAX_WORLD_EVENT_MESSAGE_LENGTH - 1);
    header.worldEventMessage[MAX_WORLD_EVENT_MESSAGE_LENGTH - 1] = '\0';
    const bool headerWritten = writeAllBytes(fileHandle, &header, sizeof(header));
    const bool regionsWritten = writeAllBytes(fileHandle, snapshot.regionIds.data(), snapshot.regionIds.size());
    const bool terrainsWritten = writeAllBytes(fileHandle, snapshot.terrainIds.data(), snapshot.terrainIds.size());
    const bool elevationsWritten = writeAllBytes(fileHandle, snapshot.elevations.data(), snapshot.elevations.size() * sizeof(int16_t));
    const bool flagsWritten = writeAllBytes(fileHandle, snapshot.flags.data(), snapshot.flags.size() * sizeof(uint32_t));
    const bool economicWeightsWritten = writeAllBytes(fileHandle, snapshot.economicWeights.data(), snapshot.economicWeights.size());
    const bool populationsWritten = writeAllBytes(fileHandle, snapshot.populations.data(), snapshot.populations.size() * sizeof(uint16_t));
    const bool crimePressuresWritten = writeAllBytes(fileHandle, snapshot.crimePressures.data(), snapshot.crimePressures.size());
    const bool lawPressuresWritten = writeAllBytes(fileHandle, snapshot.lawPressures.data(), snapshot.lawPressures.size());
    const bool businessVitalitiesWritten = writeAllBytes(fileHandle, snapshot.businessVitalities.data(), snapshot.businessVitalities.size());
    const bool playerInfluencesWritten = writeAllBytes(fileHandle, snapshot.playerInfluences.data(), snapshot.playerInfluences.size());
    const bool oppositionInfluencesWritten = writeAllBytes(fileHandle, snapshot.oppositionInfluences.data(), snapshot.oppositionInfluences.size());
    const bool cityOwnersWritten = writeAllBytes(fileHandle, snapshot.cityOwnerIds.data(), snapshot.cityOwnerIds.size());
    const int32_t agentCount = getCharacterAgentDefinitionCount();
    const bool catalogWritten = writeAllBytes(fileHandle, snapshot.activeCatalogIndices, sizeof(snapshot.activeCatalogIndices));
    const bool agentsWritten = writeAllBytes(fileHandle, snapshot.characterAgentStore.states, sizeof(CharacterAgentState) * static_cast<size_t>(MAX_CHARACTER_AGENT_COUNT));
    const bool agentCountWritten = writeAllBytes(fileHandle, &agentCount, sizeof(agentCount));
    const bool organizationWritten = writeAllBytes(fileHandle, &snapshot.organizationStore, sizeof(snapshot.organizationStore));
    const bool lawWritten = writeAllBytes(fileHandle, &snapshot.lawEnforcementStore, sizeof(snapshot.lawEnforcementStore));
    const bool streetCrimeWritten = writeAllBytes(fileHandle, &snapshot.streetCrimeStore, sizeof(snapshot.streetCrimeStore));
    const bool justiceWritten = writeAllBytes(fileHandle, &snapshot.criminalJusticeStore, sizeof(snapshot.criminalJusticeStore));
    const bool workExperienceWritten = writeAllBytes(fileHandle, &snapshot.workExperienceMonths, sizeof(snapshot.workExperienceMonths));
    const bool jobReapplyWritten = writeAllBytes(
        fileHandle,
        snapshot.jobReapplyAvailableTickByBusiness,
        sizeof(snapshot.jobReapplyAvailableTickByBusiness));
    const bool gameplayWritten = writeAllBytes(fileHandle, &snapshot.gameplayStores, sizeof(snapshot.gameplayStores));
    const bool didSave = headerWritten && regionsWritten && terrainsWritten && elevationsWritten && flagsWritten
        && economicWeightsWritten && populationsWritten && crimePressuresWritten && lawPressuresWritten
        && businessVitalitiesWritten && playerInfluencesWritten && oppositionInfluencesWritten && cityOwnersWritten
        && catalogWritten && agentsWritten && agentCountWritten && organizationWritten && lawWritten && streetCrimeWritten
        && justiceWritten && workExperienceWritten && jobReapplyWritten && gameplayWritten;
    std::fclose(fileHandle);
    return didSave;
}

bool loadGameFromFile(const char* filePath, SaveGameSnapshot& outSnapshot) {
    if (filePath == nullptr) {
        return false;
    }
    FILE* fileHandle = std::fopen(filePath, "rb");
    if (fileHandle == nullptr) {
        return false;
    }
    SaveGameHeader header{};
    if (!readAllBytes(fileHandle, &header, sizeof(header))) {
        std::fclose(fileHandle);
        return false;
    }
    if (std::memcmp(header.magic, SAVE_MAGIC, sizeof(SAVE_MAGIC)) != 0) {
        std::fclose(fileHandle);
        return false;
    }
    if (header.version < SAVE_VERSION_MIN || header.version > SAVE_VERSION) {
        std::fclose(fileHandle);
        return false;
    }
    if (header.worldWidthTiles != WorldConfig::WORLD_WIDTH_TILES || header.worldHeightTiles != WorldConfig::WORLD_HEIGHT_TILES) {
        std::fclose(fileHandle);
        return false;
    }
    outSnapshot.worldSeed = header.worldSeed;
    std::memcpy(outSnapshot.characterDraft.nameBuffer, header.nameBuffer, sizeof(outSnapshot.characterDraft.nameBuffer));
    outSnapshot.characterDraft.nationalityId = static_cast<NationalityId>(header.nationalityId);
    outSnapshot.characterDraft.heritageId = static_cast<HeritageId>(header.heritageId);
    outSnapshot.characterDraft.generationId = static_cast<GenerationId>(header.generationId);
    outSnapshot.characterDraft.backgroundId = static_cast<BackgroundId>(header.backgroundId);
    outSnapshot.characterDraft.age = header.age;
    outSnapshot.characterDraft.selectedBoroughIndex = header.selectedBoroughIndex;
    outSnapshot.characterDraft.startingCityLandmarkIndex = header.startingCityLandmarkIndex;
    outSnapshot.characterDraft.hasInitializedDefaults = header.hasInitializedDefaults != 0U;
    outSnapshot.characterDraft.hasFamilyInCountry = header.hasFamilyInCountry != 0U;
    outSnapshot.characterDraft.hasFriendsInCountry = header.hasFriendsInCountry != 0U;
    outSnapshot.headquartersKind = static_cast<HeadquartersKind>(header.headquartersKind);
    outSnapshot.employedBusinessIndices[0] = header.employedBusinessIndices[0];
    outSnapshot.employedBusinessIndices[1] = header.employedBusinessIndices[1];
    outSnapshot.activeOperationCount = header.activeOperationCount;
    outSnapshot.familyOpinionPenalty = header.familyOpinionPenalty;
    outSnapshot.headquartersEstablishedTick = header.headquartersEstablishedTick;
    outSnapshot.lastMonthlyLedgerTick = header.lastMonthlyLedgerTick;
    outSnapshot.lastFamilyUpkeepTick = header.lastFamilyUpkeepTick;
    outSnapshot.headquartersRegionId = header.headquartersRegionId;
    outSnapshot.consecutiveUnpaidRentMonths = header.consecutiveUnpaidRentMonths;
    outSnapshot.rentMultiplierBps = header.rentMultiplierBps;
    outSnapshot.rentEventAdjustmentBps = header.rentEventAdjustmentBps;
    outSnapshot.worldEventStore.worldFlags = header.worldEventFlags;
    outSnapshot.worldEventStore.firedOnceMask = header.worldEventFiredOnceMask;
    std::strncpy(outSnapshot.worldEventStore.lastPlayerMessage, header.worldEventMessage, MAX_WORLD_EVENT_MESSAGE_LENGTH - 1);
    outSnapshot.worldEventStore.lastPlayerMessage[MAX_WORLD_EVENT_MESSAGE_LENGTH - 1] = '\0';
    outSnapshot.cashCents = header.cashCents;
    outSnapshot.lifetimeLegitCents = header.lifetimeLegitCents;
    outSnapshot.lifetimeCrimeCents = header.lifetimeCrimeCents;
    outSnapshot.tickCount = header.tickCount;
    outSnapshot.isPaused = header.isPaused != 0U;
    outSnapshot.speedMultiplier = header.speedMultiplier;
    outSnapshot.accumulatorSeconds = header.accumulatorSeconds;
    outSnapshot.mapCamera.centerWorldX = header.centerWorldX;
    outSnapshot.mapCamera.centerWorldY = header.centerWorldY;
    outSnapshot.mapCamera.pixelsPerTile = header.pixelsPerTile;
    outSnapshot.regionIds.assign(static_cast<size_t>(SAVE_GAME_TILE_COUNT), 0U);
    outSnapshot.terrainIds.assign(static_cast<size_t>(SAVE_GAME_TILE_COUNT), 0U);
    outSnapshot.elevations.assign(static_cast<size_t>(SAVE_GAME_TILE_COUNT), 0);
    outSnapshot.flags.assign(static_cast<size_t>(SAVE_GAME_TILE_COUNT), 0U);
    outSnapshot.economicWeights.assign(static_cast<size_t>(SAVE_GAME_TILE_COUNT), 0U);
    outSnapshot.populations.assign(static_cast<size_t>(SAVE_GAME_TILE_COUNT), 0U);
    outSnapshot.crimePressures.assign(static_cast<size_t>(SAVE_GAME_TILE_COUNT), 0U);
    outSnapshot.lawPressures.assign(static_cast<size_t>(SAVE_GAME_TILE_COUNT), 0U);
    outSnapshot.businessVitalities.assign(static_cast<size_t>(SAVE_GAME_TILE_COUNT), 0U);
    outSnapshot.playerInfluences.assign(static_cast<size_t>(SAVE_GAME_TILE_COUNT), 0U);
    outSnapshot.oppositionInfluences.assign(static_cast<size_t>(SAVE_GAME_TILE_COUNT), 0U);
    outSnapshot.cityOwnerIds.assign(static_cast<size_t>(getLandmarkCount()), 0U);
    const bool regionsRead = readAllBytes(fileHandle, outSnapshot.regionIds.data(), outSnapshot.regionIds.size());
    const bool terrainsRead = readAllBytes(fileHandle, outSnapshot.terrainIds.data(), outSnapshot.terrainIds.size());
    const bool elevationsRead = readAllBytes(fileHandle, outSnapshot.elevations.data(), outSnapshot.elevations.size() * sizeof(int16_t));
    const bool flagsRead = readAllBytes(fileHandle, outSnapshot.flags.data(), outSnapshot.flags.size() * sizeof(uint32_t));
    const bool economicWeightsRead = readAllBytes(fileHandle, outSnapshot.economicWeights.data(), outSnapshot.economicWeights.size());
    const bool populationsRead = readAllBytes(fileHandle, outSnapshot.populations.data(), outSnapshot.populations.size() * sizeof(uint16_t));
    const bool crimePressuresRead = readAllBytes(fileHandle, outSnapshot.crimePressures.data(), outSnapshot.crimePressures.size());
    const bool lawPressuresRead = readAllBytes(fileHandle, outSnapshot.lawPressures.data(), outSnapshot.lawPressures.size());
    const bool businessVitalitiesRead = readAllBytes(fileHandle, outSnapshot.businessVitalities.data(), outSnapshot.businessVitalities.size());
    const bool playerInfluencesRead = readAllBytes(fileHandle, outSnapshot.playerInfluences.data(), outSnapshot.playerInfluences.size());
    const bool oppositionInfluencesRead = readAllBytes(fileHandle, outSnapshot.oppositionInfluences.data(), outSnapshot.oppositionInfluences.size());
    const bool cityOwnersRead = readAllBytes(fileHandle, outSnapshot.cityOwnerIds.data(), outSnapshot.cityOwnerIds.size());
    const bool catalogRead = readAllBytes(fileHandle, outSnapshot.activeCatalogIndices, sizeof(outSnapshot.activeCatalogIndices));
    const bool agentsRead = readAllBytes(fileHandle, outSnapshot.characterAgentStore.states, sizeof(CharacterAgentState) * static_cast<size_t>(MAX_CHARACTER_AGENT_COUNT));
    int32_t agentCount = 0;
    const bool agentCountRead = readAllBytes(fileHandle, &agentCount, sizeof(agentCount));
    (void)agentCount;
    bool organizationRead = readAllBytes(fileHandle, &outSnapshot.organizationStore, sizeof(outSnapshot.organizationStore));
    bool lawRead = readAllBytes(fileHandle, &outSnapshot.lawEnforcementStore, sizeof(outSnapshot.lawEnforcementStore));
    bool streetCrimeRead = readAllBytes(fileHandle, &outSnapshot.streetCrimeStore, sizeof(outSnapshot.streetCrimeStore));
    bool justiceRead = readAllBytes(fileHandle, &outSnapshot.criminalJusticeStore, sizeof(outSnapshot.criminalJusticeStore));
    if (!organizationRead || !lawRead || !streetCrimeRead) {
        resetPlayerOrganizationStore(outSnapshot.organizationStore);
        resetPlayerLawEnforcementStore(outSnapshot.lawEnforcementStore);
        resetPlayerStreetCrimeStore(outSnapshot.streetCrimeStore);
        organizationRead = true;
        lawRead = true;
        streetCrimeRead = true;
    }
    if (!justiceRead) {
        resetPlayerCriminalJusticeStore(outSnapshot.criminalJusticeStore);
        justiceRead = true;
    }
    bool workExperienceRead = true;
    bool gameplayRead = true;
    if (header.version >= 14U) {
        workExperienceRead = readAllBytes(fileHandle, &outSnapshot.workExperienceMonths, sizeof(outSnapshot.workExperienceMonths));
        const bool jobReapplyRead = readAllBytes(
            fileHandle,
            outSnapshot.jobReapplyAvailableTickByBusiness,
            sizeof(outSnapshot.jobReapplyAvailableTickByBusiness));
        gameplayRead = readAllBytes(fileHandle, &outSnapshot.gameplayStores, sizeof(outSnapshot.gameplayStores));
        workExperienceRead = workExperienceRead && jobReapplyRead;
    } else if (header.version >= 13U) {
        workExperienceRead = readAllBytes(fileHandle, &outSnapshot.workExperienceMonths, sizeof(outSnapshot.workExperienceMonths));
        gameplayRead = readAllBytes(fileHandle, &outSnapshot.gameplayStores, sizeof(outSnapshot.gameplayStores));
        for (int32_t businessIndex = 0; businessIndex < MAX_BUSINESS_NODE_COUNT; ++businessIndex) {
            outSnapshot.jobReapplyAvailableTickByBusiness[businessIndex] = 0ULL;
        }
    } else if (header.version >= 12U) {
        workExperienceRead = readAllBytes(fileHandle, &outSnapshot.workExperienceMonths, sizeof(outSnapshot.workExperienceMonths));
        const size_t legacyGameplayBytes = offsetof(SaveGameplayStores, informationFeedStore);
        gameplayRead = readAllBytes(fileHandle, &outSnapshot.gameplayStores, legacyGameplayBytes);
        resetPlayerInformationFeedStore(outSnapshot.gameplayStores.informationFeedStore);
    } else if (header.version >= 11U) {
        workExperienceRead = readAllBytes(fileHandle, &outSnapshot.workExperienceMonths, sizeof(outSnapshot.workExperienceMonths));
        const size_t legacyGameplayBytes = offsetof(SaveGameplayStores, narrativeArchiveStore);
        gameplayRead = readAllBytes(fileHandle, &outSnapshot.gameplayStores, legacyGameplayBytes);
        resetPlayerNarrativeArchiveStore(outSnapshot.gameplayStores.narrativeArchiveStore);
    } else {
        outSnapshot.workExperienceMonths = 0;
        resetSaveGameplayStores(outSnapshot.gameplayStores);
    }
    if (header.version < 14U) {
        for (int32_t businessIndex = 0; businessIndex < MAX_BUSINESS_NODE_COUNT; ++businessIndex) {
            outSnapshot.jobReapplyAvailableTickByBusiness[businessIndex] = 0ULL;
        }
    }
    const bool didLoad = regionsRead && terrainsRead && elevationsRead && flagsRead
        && economicWeightsRead && populationsRead && crimePressuresRead && lawPressuresRead
        && businessVitalitiesRead && playerInfluencesRead && oppositionInfluencesRead && cityOwnersRead
        && catalogRead && agentsRead && agentCountRead && organizationRead && lawRead && streetCrimeRead && justiceRead
        && workExperienceRead && gameplayRead;
    std::fclose(fileHandle);
    return didLoad;
}

} // namespace Core
