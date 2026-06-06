#pragma once

#include "game/game_calendar.h"
#include "world/tile_vitality.h"
#include <cstdint>

namespace Core {

constexpr int32_t ECONOMY_MULTIPLIER_BPS_BASE = 10000;

float getBoroughEconomyMultiplier(RegionId regionId, const BoroughVitalityStore& boroughVitalityStore);
float getYearEconomyMultiplier(const GameCalendarStore& calendarStore);
float getCombinedEconomyMultiplier(
    RegionId regionId,
    const GameCalendarStore& calendarStore,
    const BoroughVitalityStore& boroughVitalityStore);
int32_t getCombinedEconomyMultiplierBps(
    RegionId regionId,
    const GameCalendarStore& calendarStore,
    const BoroughVitalityStore& boroughVitalityStore);
int64_t scalePriceCentsByEconomy(int64_t basePriceCents, int32_t combinedMultiplierBps);

} // namespace Core
