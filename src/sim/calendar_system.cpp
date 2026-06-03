#include "sim/calendar_system.h"
#include "game/player_health.h"

namespace Core {

void CalendarSystem::bind(
    const SimWorldBindings& inputBindings,
    GameCalendarStore* inputCalendarStore,
    PlayerWorkScheduleStore* inputWorkScheduleStore,
    PlayerWorldState* inputWorldState,
    PlayerOperationsStore* inputOperationsStore,
    PlayerHealthStore* inputPlayerHealthStore,
    PopulationHealthStore* inputPopulationHealthStore,
    CharacterAgentStore* inputAgentStore) {
    bindings = inputBindings;
    calendarStore = inputCalendarStore;
    workScheduleStore = inputWorkScheduleStore;
    worldState = inputWorldState;
    operationsStore = inputOperationsStore;
    playerHealthStore = inputPlayerHealthStore;
    populationHealthStore = inputPopulationHealthStore;
    agentStore = inputAgentStore;
}

const char* CalendarSystem::getName() const {
    return "CalendarSystem";
}

void CalendarSystem::onTick(uint64_t tickCount) {
    lastTickCount = tickCount;
    if (calendarStore == nullptr) {
        return;
    }
    advanceGameCalendar(*calendarStore, 1);
    if (playerHealthStore != nullptr && bindings.worldSeed != nullptr) {
        tickPlayerHealth(*playerHealthStore, tickCount, *bindings.worldSeed);
    }
    if (populationHealthStore != nullptr && agentStore != nullptr && bindings.worldSeed != nullptr) {
        tickPopulationHealth(*populationHealthStore, *agentStore, tickCount, *bindings.worldSeed);
    }
}

} // namespace Core
