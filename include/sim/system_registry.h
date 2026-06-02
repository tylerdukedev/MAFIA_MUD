#pragma once

#include "sim/influence_system.h"
#include "sim/isim_system.h"
#include "sim/sim_context.h"
#include <cstdint>

namespace Core {

class SystemRegistry {
public:
    SystemRegistry();
    void initialize(const SimContext& simContext);
    void runTick(uint64_t tickCount);
    int32_t getSystemCount() const;
    const ISimSystem* getSystem(int32_t index) const;

private:
    DebugSystem debugSystem;
    InfluenceSystem influenceSystem;
    ISimSystem* systems[8];
    int32_t systemCount;
};

} // namespace Core
