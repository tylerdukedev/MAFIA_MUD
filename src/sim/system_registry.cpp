#include "sim/system_registry.h"

namespace Core {

SystemRegistry::SystemRegistry()
    : systemCount(0) {
    for (int32_t index = 0; index < 8; ++index) {
        systems[index] = nullptr;
    }
}

void SystemRegistry::initialize(const SimContext& simContext) {
    systemCount = 0;
    systems[systemCount++] = &debugSystem;
    influenceSystem.bindContext(simContext);
    systems[systemCount++] = &influenceSystem;
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
