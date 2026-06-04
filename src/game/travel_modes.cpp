#include "game/travel_modes.h"
#include "game/game_calendar.h"
#include "game/player_wallet.h"
#include "game/road_pathfinding.h"
#include <cstdlib>

namespace Core {

namespace {

constexpr TravelModeDefinition TRAVEL_MODE_DEFINITIONS[] = {
    {TravelMode::Walk, "Walk", 1.0f, 0, false},
    {TravelMode::Bicycle, "Bicycle", 0.55f, 25, false},
    {TravelMode::Car, "Car", 0.30f, 150, true},
    {TravelMode::Train, "Train", 0.18f, 75, true},
};

int32_t computeManhattanDistance(int32_t fromTileX, int32_t fromTileY, int32_t toTileX, int32_t toTileY) {
    const int32_t deltaX = std::abs(toTileX - fromTileX);
    const int32_t deltaY = std::abs(toTileY - fromTileY);
    return deltaX + deltaY;
}

bool buildRoadTravelPath(
    const ChunkStore& chunkStore,
    const WorldConfig& worldConfig,
    int32_t fromTileX,
    int32_t fromTileY,
    int32_t toTileX,
    int32_t toTileY,
    RoadPathResult& outPath) {
    outPath.tileCount = 0;
    return findRoadPath(chunkStore, worldConfig, fromTileX, fromTileY, toTileX, toTileY, outPath);
}

} // namespace

int32_t getTravelModeCount() {
    return static_cast<int32_t>(sizeof(TRAVEL_MODE_DEFINITIONS) / sizeof(TRAVEL_MODE_DEFINITIONS[0]));
}

const TravelModeDefinition* getTravelModeDefinition(TravelMode mode) {
    const int32_t modeIndex = static_cast<int32_t>(mode);
    if (modeIndex < 0 || modeIndex >= getTravelModeCount()) {
        return &TRAVEL_MODE_DEFINITIONS[0];
    }
    return &TRAVEL_MODE_DEFINITIONS[modeIndex];
}

int32_t computeTravelTicksForMode(TravelMode mode, int32_t manhattanTileDistance) {
    const TravelModeDefinition* definition = getTravelModeDefinition(mode);
    const float scaledTicks = static_cast<float>(manhattanTileDistance * TICKS_PER_TRAVEL_TILE) * definition->tileTicksMultiplier;
    return static_cast<int32_t>(scaledTicks < 1.0f ? 1.0f : scaledTicks);
}

void buildTravelPlan(
    TravelPlan& plan,
    TravelMode mode,
    int32_t fromTileX,
    int32_t fromTileY,
    int32_t toTileX,
    int32_t toTileY,
    const ChunkStore& chunkStore,
    const WorldConfig& worldConfig) {
    plan.mode = mode;
    plan.fromTileX = fromTileX;
    plan.fromTileY = fromTileY;
    plan.toTileX = toTileX;
    plan.toTileY = toTileY;
    RoadPathResult roadPath{};
    buildRoadTravelPath(chunkStore, worldConfig, fromTileX, fromTileY, toTileX, toTileY, roadPath);
    const int32_t pathDistance = computeRoadPathTileDistance(roadPath);
    const int32_t fallbackDistance = computeManhattanDistance(fromTileX, fromTileY, toTileX, toTileY);
    const int32_t distance = pathDistance > 0 ? pathDistance : fallbackDistance;
    plan.estimatedTicks = computeTravelTicksForMode(mode, distance);
    plan.estimatedCostCents = getTravelModeDefinition(mode)->upfrontCostCents;
}

int32_t computeTravelLeadHours(
    const ChunkStore& chunkStore,
    const WorldConfig& worldConfig,
    const PlayerWorldState& state,
    int32_t toTileX,
    int32_t toTileY,
    TravelMode mode) {
    TravelPlan plan{};
    buildTravelPlan(plan, mode, state.currentTileX, state.currentTileY, toTileX, toTileY, chunkStore, worldConfig);
    const int32_t travelHours = (plan.estimatedTicks + CALENDAR_TICKS_PER_HOUR - 1) / CALENDAR_TICKS_PER_HOUR;
    return travelHours + TRAVEL_SCHEDULE_BUFFER_HOURS;
}

bool tryExecuteTravelPlan(
    TravelPlan& plan,
    PlayerWorldState& worldState,
    RegionId targetRegionId,
    uint64_t tickCount,
    PlayerWallet& wallet,
    const ChunkStore& chunkStore,
    const WorldConfig& worldConfig) {
    const TravelModeDefinition* definition = getTravelModeDefinition(plan.mode);
    if (definition->upfrontCostCents > 0 && !tryDebitCash(wallet, definition->upfrontCostCents)) {
        return false;
    }
    if (worldState.isTraveling) {
        return false;
    }
    if (canPlayerOperateInRegion(worldState, targetRegionId)
        && worldState.currentTileX == plan.toTileX
        && worldState.currentTileY == plan.toTileY) {
        return true;
    }
    RoadPathResult roadPath{};
    if (!buildRoadTravelPath(
            chunkStore,
            worldConfig,
            worldState.currentTileX,
            worldState.currentTileY,
            plan.toTileX,
            plan.toTileY,
            roadPath)) {
        return false;
    }
    return beginPlayerTravel(
        worldState,
        plan.toTileX,
        plan.toTileY,
        targetRegionId,
        tickCount,
        plan.estimatedTicks,
        &roadPath);
}

} // namespace Core
