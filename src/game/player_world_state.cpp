#include "game/player_world_state.h"
#include "game/road_pathfinding.h"
#include <algorithm>
#include <cstdlib>

namespace Core {

namespace {

void clearTravelPath(PlayerWorldState& state) {
    state.travelPathTileCount = 0;
}

void copyTravelPath(PlayerWorldState& state, const RoadPathResult* roadPath) {
    clearTravelPath(state);
    if (roadPath == nullptr || roadPath->tileCount <= 0) {
        return;
    }
    state.travelPathTileCount = roadPath->tileCount;
    for (int32_t pathIndex = 0; pathIndex < roadPath->tileCount; ++pathIndex) {
        state.travelPathTileX[pathIndex] = roadPath->tileX[pathIndex];
        state.travelPathTileY[pathIndex] = roadPath->tileY[pathIndex];
    }
}

int32_t resolveTravelPathIndex(const PlayerWorldState& state, float travelT) {
    if (state.travelPathTileCount <= 1) {
        return 0;
    }
    const int32_t maxPathIndex = state.travelPathTileCount - 1;
    const float pathProgress = travelT * static_cast<float>(maxPathIndex);
    int32_t pathIndex = static_cast<int32_t>(pathProgress);
    if (pathIndex < 0) {
        pathIndex = 0;
    }
    if (pathIndex > maxPathIndex) {
        pathIndex = maxPathIndex;
    }
    return pathIndex;
}

void sampleTravelPathTile(
    const PlayerWorldState& state,
    float travelT,
    float& outTileX,
    float& outTileY) {
    if (state.travelPathTileCount <= 0) {
        const float originX = static_cast<float>(state.travelOriginTileX) + 0.5f;
        const float originY = static_cast<float>(state.travelOriginTileY) + 0.5f;
        outTileX = originX;
        outTileY = originY;
        return;
    }
    if (state.travelPathTileCount == 1) {
        outTileX = static_cast<float>(state.travelPathTileX[0]) + 0.5f;
        outTileY = static_cast<float>(state.travelPathTileY[0]) + 0.5f;
        return;
    }
    const int32_t lowerIndex = resolveTravelPathIndex(state, travelT);
    const int32_t upperIndex = lowerIndex >= state.travelPathTileCount - 1 ? lowerIndex : lowerIndex + 1;
    const float pathProgress = travelT * static_cast<float>(state.travelPathTileCount - 1);
    const float segmentT = pathProgress - static_cast<float>(lowerIndex);
    const float startX = static_cast<float>(state.travelPathTileX[lowerIndex]) + 0.5f;
    const float startY = static_cast<float>(state.travelPathTileY[lowerIndex]) + 0.5f;
    const float endX = static_cast<float>(state.travelPathTileX[upperIndex]) + 0.5f;
    const float endY = static_cast<float>(state.travelPathTileY[upperIndex]) + 0.5f;
    outTileX = startX + (endX - startX) * segmentT;
    outTileY = startY + (endY - startY) * segmentT;
}

void syncTravelLogicalTile(PlayerWorldState& state, float travelT) {
    if (state.travelPathTileCount <= 0) {
        return;
    }
    const int32_t pathIndex = resolveTravelPathIndex(state, travelT);
    state.currentTileX = state.travelPathTileX[pathIndex];
    state.currentTileY = state.travelPathTileY[pathIndex];
}

} // namespace

void resetPlayerWorldState(PlayerWorldState& state) {
    state = PlayerWorldState{};
}

void initializePlayerWorldStateFromStart(PlayerWorldState& state, int32_t startTileX, int32_t startTileY, RegionId regionId) {
    state.currentTileX = startTileX;
    state.currentTileY = startTileY;
    state.homeTileX = startTileX;
    state.homeTileY = startTileY;
    state.currentRegionId = static_cast<uint8_t>(regionId);
    state.hasLandlordContact = false;
    state.isAtWork = false;
    state.isTraveling = false;
    clearTravelPath(state);
}

int32_t computeTravelTicksBetweenTiles(int32_t fromTileX, int32_t fromTileY, int32_t toTileX, int32_t toTileY) {
    const int32_t deltaX = std::abs(toTileX - fromTileX);
    const int32_t deltaY = std::abs(toTileY - fromTileY);
    return (deltaX + deltaY) * TICKS_PER_TRAVEL_TILE;
}

bool canPlayerOperateInRegion(const PlayerWorldState& state, RegionId targetRegionId) {
    if (state.isTraveling) {
        return false;
    }
    if (static_cast<RegionId>(state.currentRegionId) == targetRegionId) {
        return true;
    }
    return false;
}

bool isPlayerTraveling(const PlayerWorldState& state) {
    return state.isTraveling;
}

bool beginPlayerTravel(
    PlayerWorldState& state,
    int32_t targetTileX,
    int32_t targetTileY,
    RegionId targetRegionId,
    uint64_t tickCount,
    int32_t travelDurationTicks,
    const RoadPathResult* roadPath) {
    if (state.isTraveling) {
        return false;
    }
    if (state.currentTileX == targetTileX && state.currentTileY == targetTileY) {
        state.currentRegionId = static_cast<uint8_t>(targetRegionId);
        state.lastCommuteTick = tickCount;
        state.isAtWork = false;
        clearTravelPath(state);
        return true;
    }
    if (roadPath == nullptr || roadPath->tileCount < 1) {
        return false;
    }
    const int32_t durationTicks = travelDurationTicks < 1 ? 1 : travelDurationTicks;
    copyTravelPath(state, roadPath);
    state.isTraveling = true;
    state.travelOriginTileX = state.travelPathTileX[0];
    state.travelOriginTileY = state.travelPathTileY[0];
    state.travelDestTileX = state.travelPathTileX[state.travelPathTileCount - 1];
    state.travelDestTileY = state.travelPathTileY[state.travelPathTileCount - 1];
    state.travelDestRegionId = static_cast<uint8_t>(targetRegionId);
    state.currentTileX = state.travelOriginTileX;
    state.currentTileY = state.travelOriginTileY;
    state.travelStartTick = tickCount;
    state.travelCompleteTick = tickCount + static_cast<uint64_t>(durationTicks);
    state.isAtWork = false;
    return true;
}

void tickPlayerTravel(PlayerWorldState& state, uint64_t tickCount) {
    if (!state.isTraveling) {
        return;
    }
    const float travelT = computePlayerTravelVisualT(state, tickCount);
    syncTravelLogicalTile(state, travelT);
    if (tickCount < state.travelCompleteTick) {
        return;
    }
    state.currentTileX = state.travelDestTileX;
    state.currentTileY = state.travelDestTileY;
    state.currentRegionId = state.travelDestRegionId;
    state.isTraveling = false;
    state.lastCommuteTick = tickCount;
    clearTravelPath(state);
}

float computePlayerTravelVisualT(const PlayerWorldState& state, uint64_t tickCount) {
    if (!state.isTraveling) {
        return 1.0f;
    }
    if (state.travelCompleteTick <= state.travelStartTick) {
        return 1.0f;
    }
    if (tickCount >= state.travelCompleteTick) {
        return 1.0f;
    }
    const float elapsed = static_cast<float>(tickCount - state.travelStartTick);
    const float span = static_cast<float>(state.travelCompleteTick - state.travelStartTick);
    return elapsed / span;
}

void getPlayerDisplayTile(const PlayerWorldState& state, uint64_t tickCount, float& outTileX, float& outTileY) {
    if (!state.isTraveling) {
        outTileX = static_cast<float>(state.currentTileX) + 0.5f;
        outTileY = static_cast<float>(state.currentTileY) + 0.5f;
        return;
    }
    const float travelT = computePlayerTravelVisualT(state, tickCount);
    sampleTravelPathTile(state, travelT, outTileX, outTileY);
}

bool tryTravelPlayerToTile(PlayerWorldState& state, int32_t targetTileX, int32_t targetTileY, RegionId targetRegionId, uint64_t tickCount) {
    (void)tickCount;
    state.currentTileX = targetTileX;
    state.currentTileY = targetTileY;
    state.currentRegionId = static_cast<uint8_t>(targetRegionId);
    state.isTraveling = false;
    state.lastCommuteTick = tickCount;
    state.isAtWork = false;
    clearTravelPath(state);
    return true;
}

} // namespace Core
