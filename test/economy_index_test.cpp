#include "game/economy_index.h"
#include "game/game_calendar.h"
#include <catch2/catch_test_macros.hpp>

using namespace Core;

TEST_CASE("Year economy multiplier grows with calendar year", "[economy_index]") {
    GameCalendarStore calendarStore{};
    calendarStore.year = CALENDAR_START_YEAR;
    const float baselineMultiplier = getYearEconomyMultiplier(calendarStore);
    calendarStore.year = CALENDAR_START_YEAR + 10;
    const float laterMultiplier = getYearEconomyMultiplier(calendarStore);
    REQUIRE(baselineMultiplier == 1.0f);
    REQUIRE(laterMultiplier > baselineMultiplier);
}

TEST_CASE("Borough economy multiplier tracks vitality health", "[economy_index]") {
    BoroughVitalityStore vitalityStore{};
    vitalityStore.snapshots[static_cast<int32_t>(RegionId::Manhattan)].economicHealth = 20.0f;
    const float lowHealthMultiplier = getBoroughEconomyMultiplier(RegionId::Manhattan, vitalityStore);
    vitalityStore.snapshots[static_cast<int32_t>(RegionId::Manhattan)].economicHealth = 90.0f;
    const float highHealthMultiplier = getBoroughEconomyMultiplier(RegionId::Manhattan, vitalityStore);
    REQUIRE(highHealthMultiplier > lowHealthMultiplier);
}

TEST_CASE("Combined economy multiplier scales listing prices", "[economy_index]") {
    GameCalendarStore calendarStore{};
    calendarStore.year = CALENDAR_START_YEAR + 5;
    BoroughVitalityStore vitalityStore{};
    vitalityStore.snapshots[static_cast<int32_t>(RegionId::Brooklyn)].economicHealth = 75.0f;
    const int32_t combinedBps = getCombinedEconomyMultiplierBps(RegionId::Brooklyn, calendarStore, vitalityStore);
    const int64_t scaledPrice = scalePriceCentsByEconomy(100000, combinedBps);
    REQUIRE(combinedBps > ECONOMY_MULTIPLIER_BPS_BASE);
    REQUIRE(scaledPrice > 100000);
}
