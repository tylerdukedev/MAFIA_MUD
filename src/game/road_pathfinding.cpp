#include "game/road_pathfinding.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace Core {

namespace {

constexpr int32_t PATHFIND_DIRECTION_COUNT = 4;
constexpr int32_t PATHFIND_DIRECTION_DX[PATHFIND_DIRECTION_COUNT] = {1, -1, 0, 0};
constexpr int32_t PATHFIND_DIRECTION_DY[PATHFIND_DIRECTION_COUNT] = {0, 0, 1, -1};

int32_t packTileIndex(int32_t tileX, int32_t tileY, int32_t worldWidth) {
    return tileY * worldWidth + tileX;
}

bool isWithinWorldBounds(const WorldConfig& worldConfig, int32_t tileX, int32_t tileY) {
    const WorldCoord coord{tileX, tileY};
    return worldConfig.isWithinWorldBounds(coord);
}

bool isRoadTileAt(const ChunkStore& chunkStore, const WorldConfig& worldConfig, int32_t tileX, int32_t tileY) {
    if (!isWithinWorldBounds(worldConfig, tileX, tileY)) {
        return false;
    }
    const WorldCoord coord{tileX, tileY};
    return isRoadTraversibleTerrain(chunkStore.getTerrainAt(coord));
}

int32_t manhattanDistance(int32_t fromTileX, int32_t fromTileY, int32_t toTileX, int32_t toTileY) {
    return std::abs(toTileX - fromTileX) + std::abs(toTileY - fromTileY);
}

bool reconstructPath(
    const std::vector<int32_t>& parentByPackedIndex,
    int32_t worldWidth,
    int32_t startPackedIndex,
    int32_t goalPackedIndex,
    RoadPathResult& outPath) {
    std::vector<int32_t> reversedPacked;
    int32_t cursorPackedIndex = goalPackedIndex;
    while (cursorPackedIndex >= 0) {
        reversedPacked.push_back(cursorPackedIndex);
        if (cursorPackedIndex == startPackedIndex) {
            break;
        }
        if (cursorPackedIndex >= static_cast<int32_t>(parentByPackedIndex.size())) {
            return false;
        }
        cursorPackedIndex = parentByPackedIndex[static_cast<size_t>(cursorPackedIndex)];
    }
    if (reversedPacked.empty() || reversedPacked.back() != startPackedIndex) {
        return false;
    }
    outPath.tileCount = static_cast<int32_t>(reversedPacked.size());
    if (outPath.tileCount > MAX_ROAD_PATH_TILES) {
        return false;
    }
    for (int32_t pathIndex = 0; pathIndex < outPath.tileCount; ++pathIndex) {
        const int32_t packedIndex = reversedPacked[static_cast<size_t>(outPath.tileCount - 1 - pathIndex)];
        outPath.tileX[pathIndex] = packedIndex % worldWidth;
        outPath.tileY[pathIndex] = packedIndex / worldWidth;
    }
    return true;
}

} // namespace

bool isRoadTraversibleTerrain(TerrainId terrainId) {
    return terrainId == TerrainId::Road;
}

bool findNearestRoadTile(
    const ChunkStore& chunkStore,
    const WorldConfig& worldConfig,
    int32_t fromTileX,
    int32_t fromTileY,
    int32_t& outTileX,
    int32_t& outTileY) {
    if (isRoadTileAt(chunkStore, worldConfig, fromTileX, fromTileY)) {
        outTileX = fromTileX;
        outTileY = fromTileY;
        return true;
    }
    for (int32_t radius = 1; radius <= ROAD_PATH_NEAREST_SEARCH_RADIUS; ++radius) {
        for (int32_t offsetY = -radius; offsetY <= radius; ++offsetY) {
            for (int32_t offsetX = -radius; offsetX <= radius; ++offsetX) {
                if (std::abs(offsetX) != radius && std::abs(offsetY) != radius) {
                    continue;
                }
                const int32_t candidateX = fromTileX + offsetX;
                const int32_t candidateY = fromTileY + offsetY;
                if (!isRoadTileAt(chunkStore, worldConfig, candidateX, candidateY)) {
                    continue;
                }
                outTileX = candidateX;
                outTileY = candidateY;
                return true;
            }
        }
    }
    return false;
}

bool findRoadPath(
    const ChunkStore& chunkStore,
    const WorldConfig& worldConfig,
    int32_t fromTileX,
    int32_t fromTileY,
    int32_t toTileX,
    int32_t toTileY,
    RoadPathResult& outPath) {
    outPath.tileCount = 0;
    int32_t roadStartX = fromTileX;
    int32_t roadStartY = fromTileY;
    int32_t roadGoalX = toTileX;
    int32_t roadGoalY = toTileY;
    if (!findNearestRoadTile(chunkStore, worldConfig, fromTileX, fromTileY, roadStartX, roadStartY)) {
        return false;
    }
    if (!findNearestRoadTile(chunkStore, worldConfig, toTileX, toTileY, roadGoalX, roadGoalY)) {
        return false;
    }
    if (roadStartX == roadGoalX && roadStartY == roadGoalY) {
        outPath.tileCount = 1;
        outPath.tileX[0] = roadStartX;
        outPath.tileY[0] = roadStartY;
        return true;
    }
    const int32_t worldWidth = worldConfig.WORLD_WIDTH_TILES;
    const int32_t nodeCount = worldWidth * worldConfig.WORLD_HEIGHT_TILES;
    std::vector<int32_t> parentByPackedIndex(static_cast<size_t>(nodeCount), -1);
    std::vector<int32_t> gScoreByPackedIndex(static_cast<size_t>(nodeCount), -1);
    std::vector<uint8_t> closedByPackedIndex(static_cast<size_t>(nodeCount), 0);
    std::vector<int32_t> openPackedIndices;
    openPackedIndices.reserve(256);
    const int32_t startPackedIndex = packTileIndex(roadStartX, roadStartY, worldWidth);
    const int32_t goalPackedIndex = packTileIndex(roadGoalX, roadGoalY, worldWidth);
    gScoreByPackedIndex[static_cast<size_t>(startPackedIndex)] = 0;
    openPackedIndices.push_back(startPackedIndex);
    while (!openPackedIndices.empty()) {
        int32_t bestOpenIndex = 0;
        int32_t bestPackedIndex = openPackedIndices[0];
        int32_t bestFScore = INT32_MAX;
        for (int32_t openIndex = 0; openIndex < static_cast<int32_t>(openPackedIndices.size()); ++openIndex) {
            const int32_t packedIndex = openPackedIndices[static_cast<size_t>(openIndex)];
            const int32_t tileX = packedIndex % worldWidth;
            const int32_t tileY = packedIndex / worldWidth;
            const int32_t gScore = gScoreByPackedIndex[static_cast<size_t>(packedIndex)];
            const int32_t fScore = gScore + manhattanDistance(tileX, tileY, roadGoalX, roadGoalY);
            if (fScore < bestFScore) {
                bestFScore = fScore;
                bestOpenIndex = openIndex;
                bestPackedIndex = packedIndex;
            }
        }
        openPackedIndices.erase(openPackedIndices.begin() + bestOpenIndex);
        if (closedByPackedIndex[static_cast<size_t>(bestPackedIndex)] != 0) {
            continue;
        }
        closedByPackedIndex[static_cast<size_t>(bestPackedIndex)] = 1;
        if (bestPackedIndex == goalPackedIndex) {
            if (!reconstructPath(parentByPackedIndex, worldWidth, startPackedIndex, goalPackedIndex, outPath)) {
                return false;
            }
            for (int32_t segmentIndex = 1; segmentIndex < outPath.tileCount; ++segmentIndex) {
                const int32_t deltaX = std::abs(outPath.tileX[segmentIndex] - outPath.tileX[segmentIndex - 1]);
                const int32_t deltaY = std::abs(outPath.tileY[segmentIndex] - outPath.tileY[segmentIndex - 1]);
                if (deltaX + deltaY != 1) {
                    outPath.tileCount = 0;
                    return false;
                }
            }
            return true;
        }
        const int32_t currentTileX = bestPackedIndex % worldWidth;
        const int32_t currentTileY = bestPackedIndex / worldWidth;
        const int32_t currentGScore = gScoreByPackedIndex[static_cast<size_t>(bestPackedIndex)];
        for (int32_t directionIndex = 0; directionIndex < PATHFIND_DIRECTION_COUNT; ++directionIndex) {
            const int32_t neighborX = currentTileX + PATHFIND_DIRECTION_DX[directionIndex];
            const int32_t neighborY = currentTileY + PATHFIND_DIRECTION_DY[directionIndex];
            if (!isRoadTileAt(chunkStore, worldConfig, neighborX, neighborY)) {
                continue;
            }
            const int32_t neighborPackedIndex = packTileIndex(neighborX, neighborY, worldWidth);
            if (closedByPackedIndex[static_cast<size_t>(neighborPackedIndex)] != 0) {
                continue;
            }
            const int32_t tentativeGScore = currentGScore + 1;
            const int32_t existingGScore = gScoreByPackedIndex[static_cast<size_t>(neighborPackedIndex)];
            if (existingGScore >= 0 && tentativeGScore >= existingGScore) {
                continue;
            }
            parentByPackedIndex[static_cast<size_t>(neighborPackedIndex)] = bestPackedIndex;
            gScoreByPackedIndex[static_cast<size_t>(neighborPackedIndex)] = tentativeGScore;
            openPackedIndices.push_back(neighborPackedIndex);
        }
    }
    return false;
}

int32_t computeRoadPathTileDistance(const RoadPathResult& path) {
    if (path.tileCount <= 1) {
        return 0;
    }
    int32_t distance = 0;
    for (int32_t segmentIndex = 1; segmentIndex < path.tileCount; ++segmentIndex) {
        distance += manhattanDistance(
            path.tileX[segmentIndex - 1],
            path.tileY[segmentIndex - 1],
            path.tileX[segmentIndex],
            path.tileY[segmentIndex]);
    }
    return distance;
}

} // namespace Core
