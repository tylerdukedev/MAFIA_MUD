#include "world/chunk_store.h"
#include "world/region_table.h"
#include "world/world_config.h"
#include <catch2/catch_test_macros.hpp>

using namespace Core;

TEST_CASE("WorldConfig bounds checking", "[world][config]") {
    WorldConfig config;
    SECTION("Valid world coordinates are accepted") {
        WorldCoord inputCoord{0, 0};
        WorldCoord maxCoord{WorldConfig::WORLD_WIDTH_TILES - 1, WorldConfig::WORLD_HEIGHT_TILES - 1};
        REQUIRE(config.isWithinWorldBounds(inputCoord));
        REQUIRE(config.isWithinWorldBounds(maxCoord));
    }
    SECTION("Out-of-bounds world coordinates are rejected") {
        WorldCoord negativeCoord{-1, 0};
        WorldCoord overflowCoord{WorldConfig::WORLD_WIDTH_TILES, 0};
        REQUIRE_FALSE(config.isWithinWorldBounds(negativeCoord));
        REQUIRE_FALSE(config.isWithinWorldBounds(overflowCoord));
    }
}

TEST_CASE("WorldConfig coordinate conversion", "[world][config]") {
    WorldConfig config;
    SECTION("World to chunk and local conversion round-trips") {
        WorldCoord inputCoord{65, 33};
        ChunkCoord expectedChunk{2, 1};
        LocalTileCoord expectedLocal{1, 1};
        const ChunkCoord actualChunk = config.worldToChunkCoord(inputCoord);
        const LocalTileCoord actualLocal = config.worldToLocalTileCoord(inputCoord);
        REQUIRE(actualChunk.x == expectedChunk.x);
        REQUIRE(actualChunk.y == expectedChunk.y);
        REQUIRE(actualLocal.x == expectedLocal.x);
        REQUIRE(actualLocal.y == expectedLocal.y);
        const WorldCoord roundTrip = config.chunkAndLocalToWorld(actualChunk, actualLocal);
        REQUIRE(roundTrip.x == inputCoord.x);
        REQUIRE(roundTrip.y == inputCoord.y);
    }
    SECTION("Chunk index conversion round-trips") {
        ChunkCoord inputChunk{3, 4};
        const int32_t chunkIndex = config.chunkCoordToIndex(inputChunk);
        const ChunkCoord outputChunk = config.chunkIndexToCoord(chunkIndex);
        REQUIRE(outputChunk.x == inputChunk.x);
        REQUIRE(outputChunk.y == inputChunk.y);
    }
    SECTION("Local tile index matches row-major layout") {
        LocalTileCoord inputLocal{5, 7};
        const int32_t expectedIndex = 7 * WorldConfig::CHUNK_SIZE + 5;
        REQUIRE(config.localTileToIndex(inputLocal) == expectedIndex);
    }
}

TEST_CASE("ChunkStore lazy allocation", "[world][chunk]") {
    WorldConfig config;
    ChunkStore chunkStore(config);
    REQUIRE(chunkStore.getActiveChunkCount() == 0);
    SECTION("Creating a chunk activates storage") {
        ChunkCoord inputChunk{1, 1};
        const ChunkId chunkId = chunkStore.getOrCreateChunk(inputChunk);
        REQUIRE(chunkId != INVALID_CHUNK_ID);
        REQUIRE(chunkStore.getActiveChunkCount() == 1);
        const Chunk* chunk = chunkStore.getChunk(chunkId);
        REQUIRE(chunk != nullptr);
        REQUIRE(chunk->tiles.isInitialized());
        REQUIRE(WorldConfig::TILES_PER_CHUNK == WorldConfig::CHUNK_SIZE * WorldConfig::CHUNK_SIZE);
    }
    SECTION("Tile read/write respects world coordinates") {
        WorldCoord inputCoord{40, 40};
        chunkStore.setRegionAt(inputCoord, RegionId::Manhattan);
        chunkStore.setTerrainAt(inputCoord, TerrainId::Building);
        const RegionId actualRegion = chunkStore.getRegionAt(inputCoord);
        const TerrainId actualTerrain = chunkStore.getTerrainAt(inputCoord);
        REQUIRE(actualRegion == RegionId::Manhattan);
        REQUIRE(actualTerrain == TerrainId::Building);
        REQUIRE(chunkStore.getActiveChunkCount() == 1);
    }
    SECTION("Out-of-bounds access is safe") {
        WorldCoord outOfBounds{WorldConfig::WORLD_WIDTH_TILES, 0};
        chunkStore.setRegionAt(outOfBounds, RegionId::Brooklyn);
        const RegionId actualRegion = chunkStore.getRegionAt(outOfBounds);
        REQUIRE(actualRegion == RegionId::None);
        ChunkCoord invalidChunk{WorldConfig::CHUNK_COUNT_X, 0};
        REQUIRE(chunkStore.getOrCreateChunk(invalidChunk) == INVALID_CHUNK_ID);
    }
}

TEST_CASE("RegionTable names", "[world][region]") {
    REQUIRE(RegionTable::getRegionName(RegionId::Manhattan) == "Manhattan");
    REQUIRE(RegionTable::getRegionShortName(RegionId::NewJersey) == "NJ");
    REQUIRE(RegionTable::getPlayableRegionCount() == 7);
}
