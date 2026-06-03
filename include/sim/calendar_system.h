#pragma once

#include "game/game_calendar.h"
#include "game/player_health.h"
#include "game/player_work_schedule.h"
#include "game/player_operations.h"
#include "game/player_world_state.h"
#include "sim/character_agent.h"
#include "sim/isim_system.h"
#include "sim/sim_world_bindings.h"

namespace Core {

class CalendarSystem final : public ISimSystem {
public:
    void bind(
        const SimWorldBindings& inputBindings,
        GameCalendarStore* inputCalendarStore,
        PlayerWorkScheduleStore* inputWorkScheduleStore,
        PlayerWorldState* inputWorldState,
        PlayerOperationsStore* inputOperationsStore,
        PlayerHealthStore* inputPlayerHealthStore,
        PopulationHealthStore* inputPopulationHealthStore,
        CharacterAgentStore* inputAgentStore);
    const char* getName() const override;
    void onTick(uint64_t tickCount) override;

private:
    SimWorldBindings bindings{};
    GameCalendarStore* calendarStore = nullptr;
    PlayerWorkScheduleStore* workScheduleStore = nullptr;
    PlayerWorldState* worldState = nullptr;
    PlayerOperationsStore* operationsStore = nullptr;
    PlayerHealthStore* playerHealthStore = nullptr;
    PopulationHealthStore* populationHealthStore = nullptr;
    CharacterAgentStore* agentStore = nullptr;
};

} // namespace Core
