#include "procgen/world_generator.h"
#include "world/chunk_store.h"
#include "world/world_config.h"
#include <catch2/catch_test_macros.hpp>
#include <array>

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

TEST_CASE("WorldGenerator builds all five boroughs and New Jersey", "[procgen]") {
    WorldConfig config;
    ChunkStore chunkStore(config);
    WorldGenerator generator;
    generator.generate(config, chunkStore, DEFAULT_WORLD_SEED);
    std::array<bool, static_cast<size_t>(RegionId::COUNT)> seenRegion{};
    bool hasWater = false;
    bool hasRoad = false;
    bool hasBuilding = false;
    bool hasPark = false;
    for (int32_t y = 0; y < WorldConfig::WORLD_HEIGHT_TILES; ++y) {
        for (int32_t x = 0; x < WorldConfig::WORLD_WIDTH_TILES; ++x) {
            const WorldCoord coord{x, y};
            const TerrainId terrain = chunkStore.getTerrainAt(coord);
            seenRegion[static_cast<size_t>(chunkStore.getRegionAt(coord))] = true;
            hasWater = hasWater || terrain == TerrainId::Water;
            hasRoad = hasRoad || terrain == TerrainId::Road;
            hasBuilding = hasBuilding || terrain == TerrainId::Building;
            hasPark = hasPark || terrain == TerrainId::Park;
        }
    }
    REQUIRE(seenRegion[static_cast<size_t>(RegionId::Manhattan)]);
    REQUIRE(seenRegion[static_cast<size_t>(RegionId::Brooklyn)]);
    REQUIRE(seenRegion[static_cast<size_t>(RegionId::Queens)]);
    REQUIRE(seenRegion[static_cast<size_t>(RegionId::Bronx)]);
    REQUIRE(seenRegion[static_cast<size_t>(RegionId::StatenIsland)]);
    REQUIRE(seenRegion[static_cast<size_t>(RegionId::NewJersey)]);
    REQUIRE(hasWater);
    REQUIRE(hasRoad);
    REQUIRE(hasBuilding);
    REQUIRE(hasPark);
    REQUIRE(chunkStore.getActiveChunkCount() == chunkStore.getTotalChunkCount());
}

TEST_CASE("WorldGenerator frames the map with water", "[procgen]") {
    WorldConfig config;
    ChunkStore chunkStore(config);
    WorldGenerator generator;
    generator.generate(config, chunkStore, DEFAULT_WORLD_SEED);
    const WorldCoord corners[] = {
        {1, 1},
        {WorldConfig::WORLD_WIDTH_TILES - 2, 1},
        {1, WorldConfig::WORLD_HEIGHT_TILES - 2},
        {WorldConfig::WORLD_WIDTH_TILES - 2, WorldConfig::WORLD_HEIGHT_TILES - 2},
    };
    for (const WorldCoord& corner : corners) {
        REQUIRE(chunkStore.getTerrainAt(corner) == TerrainId::Water);
    }
}
