#include "game/economy_index.h"
#include <algorithm>

namespace Core {

namespace {

constexpr int32_t BOROUGH_ECONOMY_MIN_BPS = 8200;
constexpr int32_t BOROUGH_ECONOMY_MAX_BPS = 12800;
constexpr int32_t YEAR_ECONOMY_GROWTH_BPS_PER_YEAR = 200;
constexpr int32_t YEAR_ECONOMY_MAX_BPS = 16000;

int32_t getBoroughEconomyMultiplierBps(RegionId regionId, const BoroughVitalityStore& boroughVitalityStore) {
    const BoroughVitalitySnapshot* snapshot = getBoroughSnapshot(boroughVitalityStore, regionId);
    if (snapshot == nullptr) {
        return ECONOMY_MULTIPLIER_BPS_BASE;
    }
    const float healthNormalized = std::clamp(snapshot->economicHealth / 100.0f, 0.0f, 1.0f);
    const int32_t span = BOROUGH_ECONOMY_MAX_BPS - BOROUGH_ECONOMY_MIN_BPS;
    return BOROUGH_ECONOMY_MIN_BPS + static_cast<int32_t>(static_cast<float>(span) * healthNormalized);
}

int32_t getYearEconomyMultiplierBps(const GameCalendarStore& calendarStore) {
    const int32_t yearsElapsed = std::max(0, calendarStore.year - CALENDAR_START_YEAR);
    const int32_t growthBps = yearsElapsed * YEAR_ECONOMY_GROWTH_BPS_PER_YEAR;
    return std::min(YEAR_ECONOMY_MAX_BPS, ECONOMY_MULTIPLIER_BPS_BASE + growthBps);
}

} // namespace

float getBoroughEconomyMultiplier(RegionId regionId, const BoroughVitalityStore& boroughVitalityStore) {
    return static_cast<float>(getBoroughEconomyMultiplierBps(regionId, boroughVitalityStore))
        / static_cast<float>(ECONOMY_MULTIPLIER_BPS_BASE);
}

float getYearEconomyMultiplier(const GameCalendarStore& calendarStore) {
    return static_cast<float>(getYearEconomyMultiplierBps(calendarStore)) / static_cast<float>(ECONOMY_MULTIPLIER_BPS_BASE);
}

float getCombinedEconomyMultiplier(
    RegionId regionId,
    const GameCalendarStore& calendarStore,
    const BoroughVitalityStore& boroughVitalityStore) {
    return static_cast<float>(getCombinedEconomyMultiplierBps(regionId, calendarStore, boroughVitalityStore))
        / static_cast<float>(ECONOMY_MULTIPLIER_BPS_BASE);
}

int32_t getCombinedEconomyMultiplierBps(
    RegionId regionId,
    const GameCalendarStore& calendarStore,
    const BoroughVitalityStore& boroughVitalityStore) {
    const int32_t boroughBps = getBoroughEconomyMultiplierBps(regionId, boroughVitalityStore);
    const int32_t yearBps = getYearEconomyMultiplierBps(calendarStore);
    return (boroughBps * yearBps) / ECONOMY_MULTIPLIER_BPS_BASE;
}

int64_t scalePriceCentsByEconomy(int64_t basePriceCents, int32_t combinedMultiplierBps) {
    if (basePriceCents <= 0) {
        return 0;
    }
    return (basePriceCents * static_cast<int64_t>(combinedMultiplierBps)) / static_cast<int64_t>(ECONOMY_MULTIPLIER_BPS_BASE);
}

} // namespace Core
