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
    for (int32_t index = 0; index < 10; ++index) {
        systems[index] = nullptr;
    }
}

void SystemRegistry::initialize(const SimWorldBindings& bindings, CharacterAgentStore* agentStore) {
    systemCount = 0;
    operationSystem.bind(bindings, agentStore);
    cityControlSystem.bind(bindings);
    worldEventSystem.bind(bindings);
    economySystem.bind(bindings);
    boroughVitalitySystem.bind(bindings);
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
