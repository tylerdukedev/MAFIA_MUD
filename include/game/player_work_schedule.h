#pragma once

#include "game/game_calendar.h"
#include "game/player_operations.h"
#include "game/player_world_state.h"
#include "world/chunk_store.h"
#include "world/world_config.h"
#include <cstdint>

namespace Core {

enum class WorkShiftPhase : uint8_t {
    OffDuty = 0,
    CommutePrompt = 1,
    OnShift = 2,
    ShiftComplete = 3,
};

struct PlayerWorkScheduleStore {
    WorkShiftPhase shiftPhase = WorkShiftPhase::OffDuty;
    int32_t shiftStartHour = 8;
    int32_t shiftEndHour = 17;
    int32_t hoursScheduledToday = 8;
    bool didWorkToday = false;
    bool wasLateToday = false;
    uint64_t lastShiftDayIndex = 0;
    uint64_t lastPromptHour = 0;
};

void resetPlayerWorkScheduleStore(PlayerWorkScheduleStore& store);
void tickPlayerWorkSchedule(
    PlayerWorkScheduleStore& scheduleStore,
    GameCalendarStore& calendarStore,
    PlayerWorldState& worldState,
    const PlayerOperationsStore& operationsStore,
    const ChunkStore& chunkStore,
    const WorldConfig& worldConfig,
    bool isModalActive);
bool shouldOpenWorkCommuteModal(const PlayerWorkScheduleStore& scheduleStore, const GameCalendarStore& calendarStore);
bool computeWorkDayLateness(const PlayerWorkScheduleStore& scheduleStore, const GameCalendarStore& calendarStore);
void markWorkShiftStarted(PlayerWorkScheduleStore& scheduleStore, PlayerWorldState& worldState, GameCalendarStore& calendarStore, bool isLate);
void markWorkShiftSkipped(PlayerWorkScheduleStore& scheduleStore, GameCalendarStore& calendarStore);
bool isPlayerOnWorkShift(const PlayerWorkScheduleStore& scheduleStore, const PlayerWorldState& worldState);
bool isPlayerUiRestrictedAtWork(const PlayerWorkScheduleStore& scheduleStore, const PlayerWorldState& worldState);

} // namespace Core
