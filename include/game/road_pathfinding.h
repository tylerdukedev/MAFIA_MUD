#pragma once

#include "core/types.h"
#include "world/chunk_store.h"
#include <cstdint>

namespace Core {

constexpr int32_t MAX_ROAD_PATH_TILES = 512;
constexpr int32_t ROAD_PATH_NEAREST_SEARCH_RADIUS = 32;

struct RoadPathResult {
    int32_t tileCount = 0;
    int32_t tileX[MAX_ROAD_PATH_TILES]{};
    int32_t tileY[MAX_ROAD_PATH_TILES]{};
};

bool isRoadTraversibleTerrain(TerrainId terrainId);
bool findNearestRoadTile(const ChunkStore& chunkStore, const WorldConfig& worldConfig, int32_t fromTileX, int32_t fromTileY, int32_t& outTileX, int32_t& outTileY);
bool findRoadPath(
    const ChunkStore& chunkStore,
    const WorldConfig& worldConfig,
    int32_t fromTileX,
    int32_t fromTileY,
    int32_t toTileX,
    int32_t toTileY,
    RoadPathResult& outPath);
int32_t computeRoadPathTileDistance(const RoadPathResult& path);

} // namespace Core
