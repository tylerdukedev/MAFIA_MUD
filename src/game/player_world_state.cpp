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
    state.isEmployed = false;
    state.isAtWork = false;
    state.isOnWorkShiftToday = true;
}

int32_t computeTravelTicksBetweenTiles(int32_t fromTileX, int32_t fromTileY, int32_t toTileX, int32_t toTileY) {
    const int32_t deltaX = std::abs(toTileX - fromTileX);
    const int32_t deltaY = std::abs(toTileY - fromTileY);
    return (deltaX + deltaY) * TICKS_PER_TRAVEL_TILE;
}

bool canPlayerOperateInRegion(const PlayerWorldState& state, RegionId targetRegionId) {
    if (static_cast<RegionId>(state.currentRegionId) == targetRegionId) {
        return true;
    }
    return false;
}

bool tryTravelPlayerToTile(PlayerWorldState& state, int32_t targetTileX, int32_t targetTileY, RegionId targetRegionId, uint64_t tickCount) {
    (void)tickCount;
    state.currentTileX = targetTileX;
    state.currentTileY = targetTileY;
    state.currentRegionId = static_cast<uint8_t>(targetRegionId);
    state.lastCommuteTick = tickCount;
    state.isAtWork = false;
    return true;
}

} // namespace Core
