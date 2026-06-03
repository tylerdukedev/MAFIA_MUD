#include "game/game_calendar.h"
#include "game/travel_modes.h"
#include <catch2/catch_test_macros.hpp>

using namespace Core;

TEST_CASE("Game calendar advances hours and weekdays", "[calendar]") {
    GameCalendarStore store{};
    resetGameCalendarStore(store);
    const int32_t inputTicks = CALENDAR_TICKS_PER_HOUR + 5;
    advanceGameCalendar(store, inputTicks);
    REQUIRE(store.hourOfDay == 9);
    REQUIRE(getWeekday(store) == Weekday::Monday);
}

TEST_CASE("Travel modes scale distance realistically", "[travel]") {
    const int32_t inputDistance = 20;
    const int32_t walkTicks = computeTravelTicksForMode(TravelMode::Walk, inputDistance);
    const int32_t trainTicks = computeTravelTicksForMode(TravelMode::Train, inputDistance);
    REQUIRE(walkTicks > trainTicks);
    TravelPlan plan{};
    buildTravelPlan(plan, TravelMode::Car, 10, 10, 30, 30);
    REQUIRE(plan.estimatedTicks > 0);
    REQUIRE(plan.estimatedCostCents > 0);
}
