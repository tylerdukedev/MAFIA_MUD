#include "persistence/save_game.h"
#include "world/landmark_table.h"
#include <cstdio>
#include <cstring>

namespace Core {

namespace {
constexpr char SAVE_MAGIC[4] = {'C', 'V', 'S', 'V'};
constexpr uint32_t SAVE_VERSION = 4U;

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
    const CityControlStore& cityControlStore) {
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
    CityControlStore& cityControlStore) {
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
    const bool didSave = headerWritten && regionsWritten && terrainsWritten && elevationsWritten && flagsWritten
        && economicWeightsWritten && populationsWritten && crimePressuresWritten && lawPressuresWritten
        && businessVitalitiesWritten && playerInfluencesWritten && oppositionInfluencesWritten && cityOwnersWritten;
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
    if (header.version != SAVE_VERSION) {
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
    const bool didLoad = regionsRead && terrainsRead && elevationsRead && flagsRead
        && economicWeightsRead && populationsRead && crimePressuresRead && lawPressuresRead
        && businessVitalitiesRead && playerInfluencesRead && oppositionInfluencesRead && cityOwnersRead;
    std::fclose(fileHandle);
    return didLoad;
}

} // namespace Core
