#include "procgen/world_generator.h"
#include "world/chunk_store.h"
#include "world/world_config.h"
#include <catch2/catch_test_macros.hpp>

using namespace Core;

namespace {
bool isWaterTerrain(TerrainId terrainId) {
    return terrainId == TerrainId::DeepWater || terrainId == TerrainId::ShallowWater;
}
bool isLandTerrain(TerrainId terrainId) {
    return terrainId == TerrainId::Grassland || terrainId == TerrainId::Forest
        || terrainId == TerrainId::Hills || terrainId == TerrainId::Beach;
}
bool isHighTerrain(TerrainId terrainId) {
    return terrainId == TerrainId::Mountain || terrainId == TerrainId::Peak;
}
} // namespace

TEST_CASE("WorldGenerator determinism", "[procgen]") {
    WorldConfig config;
    WorldGenerator generator;
    ChunkStore storeA(config);
    ChunkStore storeB(config);
    constexpr uint64_t inputSeed = 12345ULL;
    generator.generate(config, storeA, inputSeed);
    generator.generate(config, storeB, inputSeed);
    WorldCoord sampleCoord{200, 180};
    REQUIRE(storeA.getTerrainAt(sampleCoord) == storeB.getTerrainAt(sampleCoord));
    REQUIRE(storeA.getRegionAt(sampleCoord) == storeB.getRegionAt(sampleCoord));
    REQUIRE(storeA.getElevationAt(sampleCoord) == storeB.getElevationAt(sampleCoord));
}

TEST_CASE("WorldGenerator produces overworld terrain variety", "[procgen]") {
    WorldConfig config;
    ChunkStore chunkStore(config);
    WorldGenerator generator;
    generator.generate(config, chunkStore, DEFAULT_WORLD_SEED);
    bool hasWater = false;
    bool hasLand = false;
    bool hasHighGround = false;
    bool hasCity = false;
    bool hasRoad = false;
    bool hasRegion = false;
    for (int32_t y = 0; y < WorldConfig::WORLD_HEIGHT_TILES; ++y) {
        for (int32_t x = 0; x < WorldConfig::WORLD_WIDTH_TILES; ++x) {
            const TerrainId terrain = chunkStore.getTerrainAt(WorldCoord{x, y});
            hasWater = hasWater || isWaterTerrain(terrain);
            hasLand = hasLand || isLandTerrain(terrain);
            hasHighGround = hasHighGround || isHighTerrain(terrain);
            hasCity = hasCity || terrain == TerrainId::City;
            hasRoad = hasRoad || terrain == TerrainId::Road;
            hasRegion = hasRegion || chunkStore.getRegionAt(WorldCoord{x, y}) != RegionId::None;
        }
    }
    REQUIRE(hasWater);
    REQUIRE(hasLand);
    REQUIRE(hasHighGround);
    REQUIRE(hasCity);
    REQUIRE(hasRoad);
    REQUIRE(hasRegion);
    REQUIRE(chunkStore.getActiveChunkCount() == chunkStore.getTotalChunkCount());
}

TEST_CASE("WorldGenerator surrounds the world with ocean", "[procgen]") {
    WorldConfig config;
    ChunkStore chunkStore(config);
    WorldGenerator generator;
    generator.generate(config, chunkStore, DEFAULT_WORLD_SEED);
    const WorldCoord corners[] = {
        {0, 0},
        {WorldConfig::WORLD_WIDTH_TILES - 1, 0},
        {0, WorldConfig::WORLD_HEIGHT_TILES - 1},
        {WorldConfig::WORLD_WIDTH_TILES - 1, WorldConfig::WORLD_HEIGHT_TILES - 1},
    };
    for (const WorldCoord& corner : corners) {
        REQUIRE(isWaterTerrain(chunkStore.getTerrainAt(corner)));
        REQUIRE(chunkStore.getRegionAt(corner) == RegionId::Ocean);
    }
}
