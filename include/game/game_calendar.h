#pragma once

#include <cstdint>

namespace Core {

constexpr int32_t CALENDAR_DAYS_PER_WEEK = 7;
constexpr int32_t CALENDAR_WEEKS_PER_YEAR = 52;
constexpr int32_t CALENDAR_HOURS_PER_DAY = 24;
constexpr int32_t CALENDAR_TICKS_PER_HOUR = 100;
constexpr int32_t CALENDAR_TICKS_PER_DAY = CALENDAR_HOURS_PER_DAY * CALENDAR_TICKS_PER_HOUR;
constexpr int32_t CALENDAR_START_YEAR = 1928;

enum class Weekday : uint8_t {
    Monday = 0,
    Tuesday = 1,
    Wednesday = 2,
    Thursday = 3,
    Friday = 4,
    Saturday = 5,
    Sunday = 6,
};

struct GameCalendarStore {
    int32_t year = CALENDAR_START_YEAR;
    int32_t dayOfYear = 0;
    int32_t hourOfDay = 8;
    int32_t tickWithinHour = 0;
    int32_t totalDaysElapsed = 0;
    int32_t scheduledHoursThisWeek = 40;
    int32_t hoursWorkedThisWeek = 0;
    int32_t weekHourReduction = 0;
};

void resetGameCalendarStore(GameCalendarStore& store);
void advanceGameCalendar(GameCalendarStore& store, int32_t tickCount);
Weekday getWeekday(const GameCalendarStore& store);
int32_t getCalendarTicksIntoDay(const GameCalendarStore& store);
bool isWeekend(const GameCalendarStore& store);
bool isWorkHour(const GameCalendarStore& store, int32_t shiftStartHour, int32_t shiftEndHour);
void formatCalendarDateLabel(const GameCalendarStore& store, char* outBuffer, size_t bufferSize);
void formatWeekdayLabel(Weekday weekday, char* outBuffer, size_t bufferSize);

} // namespace Core
