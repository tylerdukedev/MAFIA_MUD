#include "sim/system_registry.h"

namespace Core {

uint64_t ISimSystem::getLastTickCount() const {
    return lastTickCount;
}

const char* DebugSystem::getName() const {
    return "DebugSystem";
}

void DebugSystem::onTick(uint64_t tickCount) {
    lastTickCount = tickCount;
    processedTickCount = tickCount;
}

uint64_t DebugSystem::getProcessedTickCount() const {
    return processedTickCount;
}

SystemRegistry::SystemRegistry()
    : systemCount(0) {
    for (int32_t index = 0; index < 13; ++index) {
        systems[index] = nullptr;
    }
}

void SystemRegistry::initialize(
    const SimWorldBindings& bindings,
    CharacterAgentStore* agentStore,
    PlayerStreetCrimeStore* crimeStore,
    PlayerLawEnforcementStore* lawStore,
    PlayerCriminalJusticeStore* justiceStore,
    GameCalendarStore* calendarStore,
    PlayerWorkScheduleStore* workScheduleStore,
    PlayerWorldState* worldState,
    PlayerHealthStore* playerHealthStore,
    PopulationHealthStore* populationHealthStore) {
    systemCount = 0;
    bindingsValid = isSimWorldBindingsValid(bindings);
    streetCrimeSystem.bind(bindings, crimeStore, lawStore, justiceStore);
    operationSystem.bind(bindings, agentStore);
    cityControlSystem.bind(bindings, justiceStore);
    worldEventSystem.bind(bindings);
    policeSystem.bind(bindings, lawStore);
    criminalJusticeSystem.bind(bindings, justiceStore, lawStore);
    economySystem.bind(bindings);
    boroughVitalitySystem.bind(bindings);
    calendarSystem.bind(
        bindings,
        calendarStore,
        workScheduleStore,
        worldState,
        bindings.playerOperationsStore,
        playerHealthStore,
        populationHealthStore,
        agentStore);
    systems[systemCount++] = &calendarSystem;
    systems[systemCount++] = &criminalJusticeSystem;
    systems[systemCount++] = &policeSystem;
    systems[systemCount++] = &streetCrimeSystem;
    systems[systemCount++] = &operationSystem;
    systems[systemCount++] = &cityControlSystem;
    systems[systemCount++] = &boroughVitalitySystem;
    systems[systemCount++] = &worldEventSystem;
    systems[systemCount++] = &economySystem;
    systems[systemCount++] = &debugSystem;
}

const BoroughVitalitySystem* SystemRegistry::getBoroughVitalitySystem() const {
    return &boroughVitalitySystem;
}

void SystemRegistry::runTick(uint64_t tickCount) {
    if (!bindingsValid) {
        return;
    }
    for (int32_t index = 0; index < systemCount; ++index) {
        if (systems[index] != nullptr) {
            systems[index]->onTick(tickCount);
        }
    }
}

int32_t SystemRegistry::getSystemCount() const {
    return systemCount;
}

const ISimSystem* SystemRegistry::getSystem(int32_t index) const {
    if (index < 0 || index >= systemCount) {
        return nullptr;
    }
    return systems[index];
}

} // namespace Core
