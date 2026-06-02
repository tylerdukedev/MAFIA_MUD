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

TEST_CASE("WorldGenerator produces city tile variety", "[procgen]") {
    WorldConfig config;
    ChunkStore chunkStore(config);
    WorldGenerator generator;
    generator.generate(config, chunkStore, DEFAULT_WORLD_SEED);
    bool hasWater = false;
    bool hasRoad = false;
    bool hasBuilding = false;
    bool hasDistrict = false;
    for (int32_t y = 17; y < WorldConfig::WORLD_HEIGHT_TILES; y += 17) {
        for (int32_t x = 19; x < WorldConfig::WORLD_WIDTH_TILES; x += 17) {
            WorldCoord coord{x, y};
            const TerrainId terrain = chunkStore.getTerrainAt(coord);
            if (terrain == TerrainId::Water) {
                hasWater = true;
            }
            if (terrain == TerrainId::Road) {
                hasRoad = true;
            }
            if (terrain == TerrainId::Building) {
                hasBuilding = true;
            }
            if (chunkStore.getRegionAt(coord) != RegionId::None) {
                hasDistrict = true;
            }
        }
    }
    REQUIRE(hasWater);
    REQUIRE(hasRoad);
    REQUIRE(hasBuilding);
    REQUIRE(hasDistrict);
    REQUIRE(chunkStore.getActiveChunkCount() == chunkStore.getTotalChunkCount());
}

TEST_CASE("WorldGenerator downtown core exists", "[procgen]") {
    WorldConfig config;
    ChunkStore chunkStore(config);
    WorldGenerator generator;
    generator.generate(config, chunkStore, DEFAULT_WORLD_SEED);
    WorldCoord downtownCoord{259, 261};
    REQUIRE(chunkStore.getRegionAt(downtownCoord) == RegionId::Downtown);
    REQUIRE(chunkStore.getTerrainAt(downtownCoord) == TerrainId::Building);
}
