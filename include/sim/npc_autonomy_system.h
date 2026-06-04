#pragma once

#include "sim/isim_system.h"
#include <cstdint>

namespace Core {

struct SimWorldBindings;
struct GameCalendarStore;
struct PropertyStore;

class NpcAutonomySystem final : public ISimSystem {
public:
    const char* getName() const override;
    void onTick(uint64_t tickCount) override;
    void setWorldSeed(uint64_t seed);
    void setDecisionInterval(int32_t ticks);
    void bind(
        const SimWorldBindings& simBindings,
        GameCalendarStore* calendarStore,
        PropertyStore* propertyStore);

private:
    uint64_t worldSeed = 0;
    int32_t decisionIntervalTicks = 50;
    int32_t ticksSinceLastDecision = 0;
    uint64_t lastProcessedTick = 0;
    const SimWorldBindings* bindings = nullptr;
    GameCalendarStore* calendarStore = nullptr;
    PropertyStore* propertyStore = nullptr;
};

} // namespace Core