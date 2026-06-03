#pragma once

#include "game/player_law_enforcement.h"
#include "sim/isim_system.h"
#include "sim/sim_world_bindings.h"

namespace Core {

class PoliceSystem : public ISimSystem {
public:
    void bind(const SimWorldBindings& inputBindings, PlayerLawEnforcementStore* inputLawStore);
    const char* getName() const override;
    void onTick(uint64_t tickCount) override;

private:
    SimWorldBindings bindings{};
    PlayerLawEnforcementStore* lawStore = nullptr;
};

} // namespace Core
