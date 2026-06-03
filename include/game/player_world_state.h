#pragma once

#include "core/types.h"
#include "world/region_table.h"
#include <cstdint>

namespace Core {

constexpr int32_t WORK_SHIFT_START_HOUR = 8;
constexpr int32_t WORK_SHIFT_END_HOUR = 17;
constexpr int32_t TICKS_PER_TRAVEL_TILE = 12;
constexpr int32_t WORK_DAY_INTERVAL_TICKS = 1600;
constexpr int32_t LATE_ARRIVAL_WAGE_PENALTY_PERCENT = 35;
struct PlayerWorldState {
    int32_t currentTileX = 0;
    int32_t currentTileY = 0;
    uint8_t currentRegionId = 0;
    int32_t homeTileX = 0;
    int32_t homeTileY = 0;
    bool hasLandlordContact = false;
    bool isEmployed = false;
    bool isAtWork = false;
    bool isOnWorkShiftToday = true;
    uint64_t lastWorkDayPromptTick = 0;
    uint64_t lastCommuteTick = 0;
};

void resetPlayerWorldState(PlayerWorldState& state);
void initializePlayerWorldStateFromStart(PlayerWorldState& state, int32_t startTileX, int32_t startTileY, RegionId regionId);
int32_t computeTravelTicksBetweenTiles(int32_t fromTileX, int32_t fromTileY, int32_t toTileX, int32_t toTileY);
bool canPlayerOperateInRegion(const PlayerWorldState& state, RegionId targetRegionId);
bool tryTravelPlayerToTile(PlayerWorldState& state, int32_t targetTileX, int32_t targetTileY, RegionId targetRegionId, uint64_t tickCount);

} // namespace Core
