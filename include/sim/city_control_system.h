#pragma once

#include "sim/isim_system.h"
#include "sim/sim_event_queue.h"
#include "sim/sim_world_bindings.h"

namespace Core {

class CityControlSystem final : public ISimSystem {
public:
    void bind(const SimWorldBindings& bindings);
    const char* getName() const override;
    void onTick(uint64_t tickCount) override;

private:
    SimWorldBindings bindings{};
    void processClaimCityEvent(const SimEvent& event, uint64_t tickCount);
};

} // namespace Core
