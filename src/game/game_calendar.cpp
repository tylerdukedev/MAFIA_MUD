#include "game/game_calendar.h"
#include <cstdio>
#include <cstring>

namespace Core {

void resetGameCalendarStore(GameCalendarStore& store) {
    store.year = CALENDAR_START_YEAR;
    store.dayOfYear = 0;
    store.hourOfDay = 8;
    store.tickWithinHour = 0;
    store.totalDaysElapsed = 0;
    store.scheduledHoursThisWeek = 40;
    store.hoursWorkedThisWeek = 0;
    store.weekHourReduction = 0;
}

Weekday getWeekday(const GameCalendarStore& store) {
    const int32_t index = store.totalDaysElapsed % CALENDAR_DAYS_PER_WEEK;
    return static_cast<Weekday>(index);
}

int32_t getCalendarTicksIntoDay(const GameCalendarStore& store) {
    return store.hourOfDay * CALENDAR_TICKS_PER_HOUR + store.tickWithinHour;
}

bool isWeekend(const GameCalendarStore& store) {
    const Weekday weekday = getWeekday(store);
    return weekday == Weekday::Saturday || weekday == Weekday::Sunday;
}

bool isWorkHour(const GameCalendarStore& store, int32_t shiftStartHour, int32_t shiftEndHour) {
    if (isWeekend(store)) {
        return false;
    }
    return store.hourOfDay >= shiftStartHour && store.hourOfDay < shiftEndHour;
}

void formatWeekdayLabel(Weekday weekday, char* outBuffer, size_t bufferSize) {
    switch (weekday) {
    case Weekday::Tuesday:
        std::snprintf(outBuffer, bufferSize, "%s", "Tuesday");
        break;
    case Weekday::Wednesday:
        std::snprintf(outBuffer, bufferSize, "%s", "Wednesday");
        break;
    case Weekday::Thursday:
        std::snprintf(outBuffer, bufferSize, "%s", "Thursday");
        break;
    case Weekday::Friday:
        std::snprintf(outBuffer, bufferSize, "%s", "Friday");
        break;
    case Weekday::Saturday:
        std::snprintf(outBuffer, bufferSize, "%s", "Saturday");
        break;
    case Weekday::Sunday:
        std::snprintf(outBuffer, bufferSize, "%s", "Sunday");
        break;
    default:
        std::snprintf(outBuffer, bufferSize, "%s", "Monday");
        break;
    }
}

void formatCalendarDateLabel(const GameCalendarStore& store, char* outBuffer, size_t bufferSize) {
    char weekdayBuffer[16];
    formatWeekdayLabel(getWeekday(store), weekdayBuffer, sizeof(weekdayBuffer));
    const int32_t weekIndex = store.totalDaysElapsed / CALENDAR_DAYS_PER_WEEK;
    std::snprintf(
        outBuffer,
        bufferSize,
        "%s, %d %d (week %d, hour %d)",
        weekdayBuffer,
        store.dayOfYear + 1,
        store.year,
        weekIndex + 1,
        store.hourOfDay);
}

void advanceGameCalendar(GameCalendarStore& store, int32_t tickCount) {
    if (tickCount <= 0) {
        return;
    }
    for (int32_t tickIndex = 0; tickIndex < tickCount; ++tickIndex) {
        store.tickWithinHour += 1;
        if (store.tickWithinHour < CALENDAR_TICKS_PER_HOUR) {
            continue;
        }
        store.tickWithinHour = 0;
        store.hourOfDay += 1;
        if (store.hourOfDay < CALENDAR_HOURS_PER_DAY) {
            continue;
        }
        store.hourOfDay = 0;
        store.dayOfYear += 1;
        store.totalDaysElapsed += 1;
        if (getWeekday(store) == Weekday::Monday && store.dayOfYear > 0) {
            store.hoursWorkedThisWeek = 0;
            store.scheduledHoursThisWeek = 40 - store.weekHourReduction;
            if (store.scheduledHoursThisWeek < 8) {
                store.scheduledHoursThisWeek = 8;
            }
            store.weekHourReduction = 0;
        }
        if (store.dayOfYear >= CALENDAR_WEEKS_PER_YEAR * CALENDAR_DAYS_PER_WEEK) {
            store.dayOfYear = 0;
            store.year += 1;
        }
    }
}

} // namespace Core
