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

TEST_CASE("WorldGenerator builds boroughs surrounded by mainland and water", "[procgen]") {
    WorldConfig config;
    ChunkStore chunkStore(config);
    WorldGenerator generator;
    generator.generate(config, chunkStore, DEFAULT_WORLD_SEED);
    std::array<bool, static_cast<size_t>(RegionId::COUNT)> seenRegion{};
    bool hasWater = false;
    bool hasRoad = false;
    bool hasBuilding = false;
    bool hasMainland = false;
    for (int32_t y = 0; y < WorldConfig::WORLD_HEIGHT_TILES; ++y) {
        for (int32_t x = 0; x < WorldConfig::WORLD_WIDTH_TILES; ++x) {
            const WorldCoord coord{x, y};
            const TerrainId terrain = chunkStore.getTerrainAt(coord);
            seenRegion[static_cast<size_t>(chunkStore.getRegionAt(coord))] = true;
            hasWater = hasWater || terrain == TerrainId::Water;
            hasRoad = hasRoad || terrain == TerrainId::Road;
            hasBuilding = hasBuilding || terrain == TerrainId::Building;
            hasMainland = hasMainland || terrain == TerrainId::OpenLand;
        }
    }
    REQUIRE(seenRegion[static_cast<size_t>(RegionId::Manhattan)]);
    REQUIRE(seenRegion[static_cast<size_t>(RegionId::Brooklyn)]);
    REQUIRE(seenRegion[static_cast<size_t>(RegionId::Queens)]);
    REQUIRE(seenRegion[static_cast<size_t>(RegionId::Bronx)]);
    REQUIRE(seenRegion[static_cast<size_t>(RegionId::StatenIsland)]);
    REQUIRE(seenRegion[static_cast<size_t>(RegionId::NewJersey)]);
    REQUIRE(seenRegion[static_cast<size_t>(RegionId::Westchester)]);
    REQUIRE(seenRegion[static_cast<size_t>(RegionId::LongIsland)]);
    REQUIRE(hasWater);
    REQUIRE(hasRoad);
    REQUIRE(hasBuilding);
    REQUIRE(hasMainland);
    REQUIRE(chunkStore.getActiveChunkCount() == chunkStore.getTotalChunkCount());
}

TEST_CASE("WorldGenerator places the Atlantic along the south edge", "[procgen]") {
    WorldConfig config;
    ChunkStore chunkStore(config);
    WorldGenerator generator;
    generator.generate(config, chunkStore, DEFAULT_WORLD_SEED);
    const WorldCoord southCenter{WorldConfig::WORLD_WIDTH_TILES / 2, WorldConfig::WORLD_HEIGHT_TILES - 2};
    REQUIRE(chunkStore.getTerrainAt(southCenter) == TerrainId::Water);
}

TEST_CASE("WorldGenerator stamps landmarks in their boroughs", "[procgen][landmark]") {
    WorldConfig config;
    ChunkStore chunkStore(config);
    WorldGenerator generator;
    generator.generate(config, chunkStore, DEFAULT_WORLD_SEED);
    bool hasCentralPark = false;
    bool hasLaGuardia = false;
    bool hasJfk = false;
    bool hasAirportTerrain = false;
    for (int32_t y = 0; y < WorldConfig::WORLD_HEIGHT_TILES; ++y) {
        for (int32_t x = 0; x < WorldConfig::WORLD_WIDTH_TILES; ++x) {
            const WorldCoord coord{x, y};
            const LandmarkId landmark = chunkStore.getLandmarkAt(coord);
            if (landmark == LandmarkId::CentralPark) {
                hasCentralPark = true;
                REQUIRE(chunkStore.getRegionAt(coord) == RegionId::Manhattan);
            }
            if (landmark == LandmarkId::LaGuardiaAirport) {
                hasLaGuardia = true;
                REQUIRE(chunkStore.getRegionAt(coord) == RegionId::Queens);
            }
            if (landmark == LandmarkId::JfkAirport) {
                hasJfk = true;
                REQUIRE(chunkStore.getRegionAt(coord) == RegionId::Queens);
            }
            if (chunkStore.getTerrainAt(coord) == TerrainId::Airport) {
                hasAirportTerrain = true;
            }
        }
    }
    REQUIRE(hasCentralPark);
    REQUIRE(hasLaGuardia);
    REQUIRE(hasJfk);
    REQUIRE(hasAirportTerrain);
}
