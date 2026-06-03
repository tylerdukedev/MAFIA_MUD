#include "game/player_work_schedule.h"
#include "game/player_employment.h"
#include <algorithm>

namespace Core {

void resetPlayerWorkScheduleStore(PlayerWorkScheduleStore& store) {
    store.shiftPhase = WorkShiftPhase::OffDuty;
    store.shiftStartHour = 8;
    store.shiftEndHour = 17;
    store.hoursScheduledToday = 8;
    store.didWorkToday = false;
    store.wasLateToday = false;
    store.lastShiftDayIndex = 0;
    store.lastPromptHour = 0;
}

bool shouldOpenWorkCommuteModal(const PlayerWorkScheduleStore& scheduleStore, const GameCalendarStore& calendarStore) {
    return scheduleStore.shiftPhase == WorkShiftPhase::CommutePrompt && !isWeekend(calendarStore);
}

bool computeWorkDayLateness(const PlayerWorkScheduleStore& scheduleStore, const GameCalendarStore& calendarStore) {
    return calendarStore.hourOfDay > scheduleStore.shiftStartHour + 1;
}

void markWorkShiftStarted(PlayerWorkScheduleStore& scheduleStore, PlayerWorldState& worldState, GameCalendarStore& calendarStore, bool isLate) {
    scheduleStore.shiftPhase = WorkShiftPhase::OnShift;
    scheduleStore.didWorkToday = true;
    scheduleStore.wasLateToday = isLate;
    worldState.isAtWork = true;
    worldState.lastWorkDayPromptTick = static_cast<uint64_t>(calendarStore.totalDaysElapsed);
    calendarStore.hoursWorkedThisWeek += scheduleStore.hoursScheduledToday;
}

void markWorkShiftSkipped(PlayerWorkScheduleStore& scheduleStore, GameCalendarStore& calendarStore) {
    scheduleStore.shiftPhase = WorkShiftPhase::ShiftComplete;
    scheduleStore.didWorkToday = false;
    (void)calendarStore;
}

void tickPlayerWorkSchedule(
    PlayerWorkScheduleStore& scheduleStore,
    GameCalendarStore& calendarStore,
    PlayerWorldState& worldState,
    const PlayerOperationsStore& operationsStore,
    bool isModalActive) {
    if (!isPlayerEmployed(operationsStore)) {
        scheduleStore.shiftPhase = WorkShiftPhase::OffDuty;
        worldState.isAtWork = false;
        return;
    }
    const uint64_t dayIndex = static_cast<uint64_t>(calendarStore.totalDaysElapsed);
    if (scheduleStore.lastShiftDayIndex != dayIndex) {
        scheduleStore.lastShiftDayIndex = dayIndex;
        scheduleStore.didWorkToday = false;
        scheduleStore.wasLateToday = false;
        scheduleStore.shiftPhase = WorkShiftPhase::OffDuty;
        scheduleStore.hoursScheduledToday = std::max(4, calendarStore.scheduledHoursThisWeek / 5);
        const int32_t endHour = scheduleStore.shiftStartHour + scheduleStore.hoursScheduledToday;
        scheduleStore.shiftEndHour = endHour > CALENDAR_HOURS_PER_DAY ? CALENDAR_HOURS_PER_DAY : endHour;
    }
    if (isWeekend(calendarStore)) {
        scheduleStore.shiftPhase = WorkShiftPhase::OffDuty;
        worldState.isAtWork = false;
        return;
    }
    if (scheduleStore.shiftPhase == WorkShiftPhase::OnShift && calendarStore.hourOfDay >= scheduleStore.shiftEndHour) {
        scheduleStore.shiftPhase = WorkShiftPhase::ShiftComplete;
        worldState.isAtWork = false;
    }
    if (scheduleStore.shiftPhase == WorkShiftPhase::ShiftComplete || scheduleStore.shiftPhase == WorkShiftPhase::OnShift) {
        return;
    }
    if (isModalActive) {
        return;
    }
    if (calendarStore.hourOfDay == scheduleStore.shiftStartHour && scheduleStore.lastPromptHour != static_cast<uint64_t>(calendarStore.hourOfDay)) {
        scheduleStore.lastPromptHour = static_cast<uint64_t>(calendarStore.hourOfDay);
        scheduleStore.shiftPhase = WorkShiftPhase::CommutePrompt;
        return;
    }
    if (calendarStore.hourOfDay > scheduleStore.shiftEndHour) {
        scheduleStore.shiftPhase = WorkShiftPhase::ShiftComplete;
    }
}

bool isPlayerOnWorkShift(const PlayerWorkScheduleStore& scheduleStore, const PlayerWorldState& worldState) {
    return worldState.isAtWork && scheduleStore.shiftPhase == WorkShiftPhase::OnShift;
}

bool isPlayerUiRestrictedAtWork(const PlayerWorkScheduleStore& scheduleStore, const PlayerWorldState& worldState) {
    return isPlayerOnWorkShift(scheduleStore, worldState);
}

} // namespace Core
