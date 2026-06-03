#pragma once

#include "game/player_world_state.h"
#include "game/player_wallet.h"
#include <cstdint>

namespace Core {

enum class TravelMode : uint8_t {
    Walk = 0,
    Bicycle = 1,
    Car = 2,
    Train = 3,
};

struct TravelModeDefinition {
    TravelMode mode;
    const char* displayName;
    float tileTicksMultiplier;
    int64_t upfrontCostCents;
    bool requiresCashOnHand;
};

struct TravelPlan {
    TravelMode mode = TravelMode::Walk;
    int32_t fromTileX = 0;
    int32_t fromTileY = 0;
    int32_t toTileX = 0;
    int32_t toTileY = 0;
    int32_t estimatedTicks = 0;
    int64_t estimatedCostCents = 0;
};

int32_t getTravelModeCount();
const TravelModeDefinition* getTravelModeDefinition(TravelMode mode);
int32_t computeTravelTicksForMode(TravelMode mode, int32_t manhattanTileDistance);
void buildTravelPlan(TravelPlan& plan, TravelMode mode, int32_t fromTileX, int32_t fromTileY, int32_t toTileX, int32_t toTileY);
bool tryExecuteTravelPlan(TravelPlan& plan, PlayerWorldState& worldState, RegionId targetRegionId, uint64_t tickCount, PlayerWallet& wallet);

} // namespace Core
