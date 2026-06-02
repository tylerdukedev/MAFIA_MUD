#include "sim/isim_system.h"

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

} // namespace Core
