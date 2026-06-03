#pragma once

#include "sim/isim_system.h"
#include "sim/sim_world_bindings.h"
#include <cstdint>

namespace Core {

class BoroughVitalitySystem final : public ISimSystem {
public:
    void bind(const SimWorldBindings& bindings);
    const char* getName() const override;
    void onTick(uint64_t tickCount) override;
    uint64_t getLastRollupTickCount() const;

private:
    SimWorldBindings bindings{};
    uint64_t lastRollupTickCount = 0;
};

} // namespace Core
