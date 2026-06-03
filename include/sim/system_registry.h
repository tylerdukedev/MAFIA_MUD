#pragma once

#include "sim/borough_vitality_system.h"
#include "sim/isim_system.h"
#include "sim/sim_world_bindings.h"
#include <cstdint>

namespace Core {

class SystemRegistry {
public:
    SystemRegistry();
    void initialize(const SimWorldBindings& bindings);
    void runTick(uint64_t tickCount);
    int32_t getSystemCount() const;
    const ISimSystem* getSystem(int32_t index) const;
    const BoroughVitalitySystem* getBoroughVitalitySystem() const;

private:
    DebugSystem debugSystem;
    BoroughVitalitySystem boroughVitalitySystem;
    ISimSystem* systems[8];
    int32_t systemCount;
};

} // namespace Core
