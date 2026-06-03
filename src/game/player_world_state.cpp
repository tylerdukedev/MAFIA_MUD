#include "game/player_world_state.h"
#include <algorithm>
#include <cstdlib>

namespace Core {

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
    int32_t travelDurationTicks) {
    if (state.isTraveling) {
        return false;
    }
    if (state.currentTileX == targetTileX && state.currentTileY == targetTileY) {
        state.currentRegionId = static_cast<uint8_t>(targetRegionId);
        state.lastCommuteTick = tickCount;
        state.isAtWork = false;
        return true;
    }
    const int32_t durationTicks = travelDurationTicks < 1 ? 1 : travelDurationTicks;
    state.isTraveling = true;
    state.travelOriginTileX = state.currentTileX;
    state.travelOriginTileY = state.currentTileY;
    state.travelDestTileX = targetTileX;
    state.travelDestTileY = targetTileY;
    state.travelDestRegionId = static_cast<uint8_t>(targetRegionId);
    state.travelStartTick = tickCount;
    state.travelCompleteTick = tickCount + static_cast<uint64_t>(durationTicks);
    state.isAtWork = false;
    return true;
}

void tickPlayerTravel(PlayerWorldState& state, uint64_t tickCount) {
    if (!state.isTraveling) {
        return;
    }
    if (tickCount < state.travelCompleteTick) {
        return;
    }
    state.currentTileX = state.travelDestTileX;
    state.currentTileY = state.travelDestTileY;
    state.currentRegionId = state.travelDestRegionId;
    state.isTraveling = false;
    state.lastCommuteTick = tickCount;
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
    const float originX = static_cast<float>(state.travelOriginTileX) + 0.5f;
    const float originY = static_cast<float>(state.travelOriginTileY) + 0.5f;
    const float destX = static_cast<float>(state.travelDestTileX) + 0.5f;
    const float destY = static_cast<float>(state.travelDestTileY) + 0.5f;
    outTileX = originX + (destX - originX) * travelT;
    outTileY = originY + (destY - originY) * travelT;
}

bool tryTravelPlayerToTile(PlayerWorldState& state, int32_t targetTileX, int32_t targetTileY, RegionId targetRegionId, uint64_t tickCount) {
    (void)tickCount;
    state.currentTileX = targetTileX;
    state.currentTileY = targetTileY;
    state.currentRegionId = static_cast<uint8_t>(targetRegionId);
    state.isTraveling = false;
    state.lastCommuteTick = tickCount;
    state.isAtWork = false;
    return true;
}

} // namespace Core
