#include "procgen/world_generator.h"
#include "world/chunk_store.h"
#include "world/world_config.h"
#include <catch2/catch_test_macros.hpp>

using namespace Core;

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

TEST_CASE("WorldGenerator produces water and land", "[procgen]") {
    WorldConfig config;
    ChunkStore chunkStore(config);
    WorldGenerator generator;
    generator.generate(config, chunkStore, DEFAULT_WORLD_SEED);
    bool hasWater = false;
    bool hasLand = false;
    bool hasRegion = false;
    for (int32_t y = 0; y < WorldConfig::WORLD_HEIGHT_TILES; y += 32) {
        for (int32_t x = 0; x < WorldConfig::WORLD_WIDTH_TILES; x += 32) {
            WorldCoord coord{x, y};
            const TerrainId terrain = chunkStore.getTerrainAt(coord);
            if (terrain == TerrainId::Water) {
                hasWater = true;
            }
            if (terrain == TerrainId::Land) {
                hasLand = true;
            }
            if (chunkStore.getRegionAt(coord) != RegionId::None) {
                hasRegion = true;
            }
        }
    }
    REQUIRE(hasWater);
    REQUIRE(hasLand);
    REQUIRE(hasRegion);
    REQUIRE(chunkStore.getActiveChunkCount() == chunkStore.getTotalChunkCount());
}

TEST_CASE("WorldGenerator Manhattan region exists", "[procgen]") {
    WorldConfig config;
    ChunkStore chunkStore(config);
    WorldGenerator generator;
    generator.generate(config, chunkStore, DEFAULT_WORLD_SEED);
    WorldCoord manhattanCoord{175, 210};
    REQUIRE(chunkStore.getRegionAt(manhattanCoord) == RegionId::Manhattan);
    REQUIRE(chunkStore.getTerrainAt(manhattanCoord) == TerrainId::Land);
}
