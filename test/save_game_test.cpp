#include "persistence/save_game.h"
#include "procgen/world_generator.h"
#include "world/tile_vitality.h"
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
    SaveGameSnapshot snapshot{};
    REQUIRE(buildSaveSnapshot(snapshot, DEFAULT_WORLD_SEED, sourceDraft, sourceClock, sourceCamera, sourceStore, sourceVitality));
    REQUIRE(saveGameToFile(TEST_SAVE_PATH, snapshot));
    REQUIRE(saveFileExists(TEST_SAVE_PATH));
    SaveGameSnapshot loadedSnapshot{};
    REQUIRE(loadGameFromFile(TEST_SAVE_PATH, loadedSnapshot));
    ChunkStore loadedStore(config);
    SimClock loadedClock(WorldConfig::DEFAULT_TICK_RATE_HZ);
    MapCamera loadedCamera{};
    CharacterDraft loadedDraft{};
    uint64_t loadedSeed = 0;
    REQUIRE(applySaveSnapshot(loadedSnapshot, loadedSeed, loadedDraft, loadedClock, loadedCamera, loadedStore));
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
    removeTestSaveFile();
}

TEST_CASE("SaveGame rejects missing save file", "[persistence]") {
    removeTestSaveFile();
    SaveGameSnapshot snapshot{};
    REQUIRE_FALSE(loadGameFromFile(TEST_SAVE_PATH, snapshot));
    REQUIRE_FALSE(saveFileExists(TEST_SAVE_PATH));
}
