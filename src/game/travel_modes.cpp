#include "game/travel_modes.h"
#include "game/player_wallet.h"
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

void buildTravelPlan(TravelPlan& plan, TravelMode mode, int32_t fromTileX, int32_t fromTileY, int32_t toTileX, int32_t toTileY) {
    plan.mode = mode;
    plan.fromTileX = fromTileX;
    plan.fromTileY = fromTileY;
    plan.toTileX = toTileX;
    plan.toTileY = toTileY;
    const int32_t distance = computeManhattanDistance(fromTileX, fromTileY, toTileX, toTileY);
    plan.estimatedTicks = computeTravelTicksForMode(mode, distance);
    plan.estimatedCostCents = getTravelModeDefinition(mode)->upfrontCostCents;
}

bool tryExecuteTravelPlan(TravelPlan& plan, PlayerWorldState& worldState, RegionId targetRegionId, uint64_t tickCount, PlayerWallet& wallet) {
    const TravelModeDefinition* definition = getTravelModeDefinition(plan.mode);
    if (definition->upfrontCostCents > 0 && !tryDebitCash(wallet, definition->upfrontCostCents)) {
        return false;
    }
    if (!canPlayerOperateInRegion(worldState, targetRegionId)) {
        return false;
    }
    return tryTravelPlayerToTile(worldState, plan.toTileX, plan.toTileY, targetRegionId, tickCount);
}

} // namespace Core
