#include "persistence/save_game.h"
#include <cstdio>
#include <cstring>

namespace Core {

namespace {
constexpr char SAVE_MAGIC[4] = {'C', 'V', 'S', 'V'};
constexpr uint32_t SAVE_VERSION = 3U;

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
    uint8_t hasInitializedDefaults;
    uint8_t headerPadding[3];
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
    const TileFieldStore& tileFieldStore,
    const DistrictStore& districtStore) {
    outSnapshot.worldSeed = worldSeed;
    outSnapshot.characterDraft = characterDraft;
    outSnapshot.tickCount = simClock.getTickCount();
    outSnapshot.isPaused = simClock.isPaused();
    outSnapshot.speedMultiplier = simClock.getSpeedMultiplier();
    outSnapshot.accumulatorSeconds = simClock.getAccumulatorSeconds();
    outSnapshot.mapCamera = mapCamera;
    outSnapshot.regionIds.assign(static_cast<size_t>(SAVE_GAME_TILE_COUNT), 0U);
    outSnapshot.terrainIds.assign(static_cast<size_t>(SAVE_GAME_TILE_COUNT), 0U);
    outSnapshot.elevations.assign(static_cast<size_t>(SAVE_GAME_TILE_COUNT), 0);
    outSnapshot.flags.assign(static_cast<size_t>(SAVE_GAME_TILE_COUNT), 0U);
    outSnapshot.tileInfluence.assign(static_cast<size_t>(TILE_FIELD_COUNT), 0.0f);
    outSnapshot.tileHeat.assign(static_cast<size_t>(TILE_FIELD_COUNT), 0.0f);
    outSnapshot.districtHeat.assign(static_cast<size_t>(DISTRICT_CELL_COUNT), 0.0f);
    outSnapshot.districtStability.assign(static_cast<size_t>(DISTRICT_CELL_COUNT), 0.0f);
    outSnapshot.districtPlayerInfluence.assign(static_cast<size_t>(DISTRICT_CELL_COUNT), 0.0f);
    if (!chunkStore.exportFullWorldTiles(
            outSnapshot.regionIds.data(),
            outSnapshot.terrainIds.data(),
            outSnapshot.elevations.data(),
            outSnapshot.flags.data(),
            SAVE_GAME_TILE_COUNT)) {
        return false;
    }
    if (!tileFieldStore.exportFields(outSnapshot.tileInfluence.data(), outSnapshot.tileHeat.data(), TILE_FIELD_COUNT)) {
        return false;
    }
    return districtStore.exportSimFields(
        outSnapshot.districtHeat.data(),
        outSnapshot.districtStability.data(),
        outSnapshot.districtPlayerInfluence.data(),
        DISTRICT_CELL_COUNT);
}

bool applySaveSnapshot(
    const SaveGameSnapshot& snapshot,
    uint64_t& outWorldSeed,
    CharacterDraft& outCharacterDraft,
    SimClock& simClock,
    MapCamera& mapCamera,
    ChunkStore& chunkStore,
    TileFieldStore& tileFieldStore,
    DistrictStore& districtStore) {
    if (static_cast<int32_t>(snapshot.regionIds.size()) != SAVE_GAME_TILE_COUNT
        || static_cast<int32_t>(snapshot.terrainIds.size()) != SAVE_GAME_TILE_COUNT
        || static_cast<int32_t>(snapshot.elevations.size()) != SAVE_GAME_TILE_COUNT
        || static_cast<int32_t>(snapshot.flags.size()) != SAVE_GAME_TILE_COUNT
        || static_cast<int32_t>(snapshot.tileInfluence.size()) != TILE_FIELD_COUNT
        || static_cast<int32_t>(snapshot.tileHeat.size()) != TILE_FIELD_COUNT
        || static_cast<int32_t>(snapshot.districtHeat.size()) != DISTRICT_CELL_COUNT
        || static_cast<int32_t>(snapshot.districtStability.size()) != DISTRICT_CELL_COUNT
        || static_cast<int32_t>(snapshot.districtPlayerInfluence.size()) != DISTRICT_CELL_COUNT) {
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
    districtStore.buildFromChunkStore(chunkStore);
    if (!districtStore.importSimFields(
            snapshot.districtHeat.data(),
            snapshot.districtStability.data(),
            snapshot.districtPlayerInfluence.data(),
            DISTRICT_CELL_COUNT)) {
        return false;
    }
    if (!tileFieldStore.importFields(snapshot.tileInfluence.data(), snapshot.tileHeat.data(), TILE_FIELD_COUNT)) {
        return false;
    }
    outWorldSeed = snapshot.worldSeed;
    outCharacterDraft = snapshot.characterDraft;
    simClock.restoreSnapshot(snapshot.tickCount, snapshot.isPaused, snapshot.speedMultiplier, snapshot.accumulatorSeconds);
    mapCamera = snapshot.mapCamera;
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
    header.hasInitializedDefaults = snapshot.characterDraft.hasInitializedDefaults ? 1U : 0U;
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
    const bool tileInfluenceWritten = writeAllBytes(
        fileHandle,
        snapshot.tileInfluence.data(),
        snapshot.tileInfluence.size() * sizeof(float));
    const bool tileHeatWritten = writeAllBytes(fileHandle, snapshot.tileHeat.data(), snapshot.tileHeat.size() * sizeof(float));
    const bool districtHeatWritten = writeAllBytes(
        fileHandle,
        snapshot.districtHeat.data(),
        snapshot.districtHeat.size() * sizeof(float));
    const bool districtStabilityWritten = writeAllBytes(
        fileHandle,
        snapshot.districtStability.data(),
        snapshot.districtStability.size() * sizeof(float));
    const bool districtInfluenceWritten = writeAllBytes(
        fileHandle,
        snapshot.districtPlayerInfluence.data(),
        snapshot.districtPlayerInfluence.size() * sizeof(float));
    const bool didSave = headerWritten && regionsWritten && terrainsWritten && elevationsWritten && flagsWritten
        && tileInfluenceWritten && tileHeatWritten && districtHeatWritten && districtStabilityWritten
        && districtInfluenceWritten;
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
    outSnapshot.characterDraft.hasInitializedDefaults = header.hasInitializedDefaults != 0U;
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
    outSnapshot.tileInfluence.assign(static_cast<size_t>(TILE_FIELD_COUNT), 0.0f);
    outSnapshot.tileHeat.assign(static_cast<size_t>(TILE_FIELD_COUNT), 0.0f);
    outSnapshot.districtHeat.assign(static_cast<size_t>(DISTRICT_CELL_COUNT), 0.0f);
    outSnapshot.districtStability.assign(static_cast<size_t>(DISTRICT_CELL_COUNT), 0.0f);
    outSnapshot.districtPlayerInfluence.assign(static_cast<size_t>(DISTRICT_CELL_COUNT), 0.0f);
    const bool regionsRead = readAllBytes(fileHandle, outSnapshot.regionIds.data(), outSnapshot.regionIds.size());
    const bool terrainsRead = readAllBytes(fileHandle, outSnapshot.terrainIds.data(), outSnapshot.terrainIds.size());
    const bool elevationsRead = readAllBytes(fileHandle, outSnapshot.elevations.data(), outSnapshot.elevations.size() * sizeof(int16_t));
    const bool flagsRead = readAllBytes(fileHandle, outSnapshot.flags.data(), outSnapshot.flags.size() * sizeof(uint32_t));
    const bool tileInfluenceRead = readAllBytes(
        fileHandle,
        outSnapshot.tileInfluence.data(),
        outSnapshot.tileInfluence.size() * sizeof(float));
    const bool tileHeatRead = readAllBytes(fileHandle, outSnapshot.tileHeat.data(), outSnapshot.tileHeat.size() * sizeof(float));
    const bool districtHeatRead = readAllBytes(
        fileHandle,
        outSnapshot.districtHeat.data(),
        outSnapshot.districtHeat.size() * sizeof(float));
    const bool districtStabilityRead = readAllBytes(
        fileHandle,
        outSnapshot.districtStability.data(),
        outSnapshot.districtStability.size() * sizeof(float));
    const bool districtInfluenceRead = readAllBytes(
        fileHandle,
        outSnapshot.districtPlayerInfluence.data(),
        outSnapshot.districtPlayerInfluence.size() * sizeof(float));
    const bool didLoad = regionsRead && terrainsRead && elevationsRead && flagsRead && tileInfluenceRead && tileHeatRead
        && districtHeatRead && districtStabilityRead && districtInfluenceRead;
    std::fclose(fileHandle);
    return didLoad;
}

} // namespace Core
