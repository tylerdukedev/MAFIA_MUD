#include "persistence/save_game.h"
#include "procgen/world_generator.h"
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
    CharacterCreationState sourceCharacter{};
    std::snprintf(sourceCharacter.nameBuffer, sizeof(sourceCharacter.nameBuffer), "Test Boss");
    sourceCharacter.selectedBackgroundIndex = 1;
    sourceCharacter.selectedBoroughIndex = 2;
    sourceCharacter.hasInitializedDefaults = true;
    SimClock sourceClock(WorldConfig::DEFAULT_TICK_RATE_HZ);
    sourceClock.restoreSnapshot(42U, false, 2.0, 0.05);
    MapCamera sourceCamera{};
    sourceCamera.centerWorldX = 120.0f;
    sourceCamera.centerWorldY = 88.0f;
    sourceCamera.pixelsPerTile = 3.5f;
    SaveGameSnapshot snapshot{};
    REQUIRE(buildSaveSnapshot(snapshot, DEFAULT_WORLD_SEED, sourceCharacter, sourceClock, sourceCamera, sourceStore));
    REQUIRE(saveGameToFile(TEST_SAVE_PATH, snapshot));
    REQUIRE(saveFileExists(TEST_SAVE_PATH));
    SaveGameSnapshot loadedSnapshot{};
    REQUIRE(loadGameFromFile(TEST_SAVE_PATH, loadedSnapshot));
    ChunkStore loadedStore(config);
    SimClock loadedClock(WorldConfig::DEFAULT_TICK_RATE_HZ);
    MapCamera loadedCamera{};
    CharacterCreationState loadedCharacter{};
    uint64_t loadedSeed = 0;
    REQUIRE(applySaveSnapshot(loadedSnapshot, loadedSeed, loadedCharacter, loadedClock, loadedCamera, loadedStore));
    const WorldCoord sampleCoord{200, 180};
    REQUIRE(loadedSeed == DEFAULT_WORLD_SEED);
    REQUIRE(loadedCharacter.selectedBackgroundIndex == sourceCharacter.selectedBackgroundIndex);
    REQUIRE(loadedCharacter.selectedBoroughIndex == sourceCharacter.selectedBoroughIndex);
    REQUIRE(std::string(loadedCharacter.nameBuffer) == std::string(sourceCharacter.nameBuffer));
    REQUIRE(loadedClock.getTickCount() == sourceClock.getTickCount());
    REQUIRE(loadedClock.isPaused() == sourceClock.isPaused());
    REQUIRE(loadedClock.getSpeedMultiplier() == sourceClock.getSpeedMultiplier());
    REQUIRE(loadedStore.getTerrainAt(sampleCoord) == sourceStore.getTerrainAt(sampleCoord));
    REQUIRE(loadedStore.getRegionAt(sampleCoord) == sourceStore.getRegionAt(sampleCoord));
    REQUIRE(loadedStore.getElevationAt(sampleCoord) == sourceStore.getElevationAt(sampleCoord));
    removeTestSaveFile();
}

TEST_CASE("SaveGame rejects missing save file", "[persistence]") {
    removeTestSaveFile();
    SaveGameSnapshot snapshot{};
    REQUIRE_FALSE(loadGameFromFile(TEST_SAVE_PATH, snapshot));
    REQUIRE_FALSE(saveFileExists(TEST_SAVE_PATH));
}
