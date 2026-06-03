#pragma once

#include "sim/isim_system.h"
#include "sim/sim_world_bindings.h"

namespace Core {

class EconomySystem final : public ISimSystem {
public:
    void bind(const SimWorldBindings& bindings);
    const char* getName() const override;
    void onTick(uint64_t tickCount) override;

private:
    SimWorldBindings bindings{};
    bool hasAppliedStartingInfluence = false;
    void recomputeIncomeRates();
    void applyAccruedIncome();
    void applyStartingBoroughInfluence();
    void applyMonthlyLivingCosts(uint64_t tickCount);
};

} // namespace Core
