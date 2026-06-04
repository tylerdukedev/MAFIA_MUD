#include "sim/npc_autonomy_system.h"
#include "sim/sim_world_bindings.h"
#include "game/npc_decision.h"
#include "world/chunk_store.h"

namespace Core {

void NpcAutonomySystem::setWorldSeed(uint64_t seed) {
    worldSeed = seed;
}

void NpcAutonomySystem::setDecisionInterval(int32_t ticks) {
    decisionIntervalTicks = ticks;
}

void NpcAutonomySystem::bind(
    const SimWorldBindings& simBindings,
    GameCalendarStore* inputCalendarStore,
    PropertyStore* inputPropertyStore) {
    bindings = &simBindings;
    calendarStore = inputCalendarStore;
    propertyStore = inputPropertyStore;
}

const char* NpcAutonomySystem::getName() const {
    return "NpcAutonomy";
}

void NpcAutonomySystem::onTick(uint64_t tickCount) {
    lastTickCount = tickCount;
    ticksSinceLastDecision += 1;

    if (ticksSinceLastDecision < decisionIntervalTicks) {
        return;
    }

    ticksSinceLastDecision = 0;
    lastProcessedTick = tickCount;

    if (bindings == nullptr || bindings->characterAgentStore == nullptr) {
        return;
    }

    NPCDecisionContext context{};
    context.tickCount = tickCount;
    context.worldSeed = worldSeed;
    context.calendarStore = calendarStore;
    context.propertyStore = propertyStore;

    tickNpcDecisions(
        *bindings->characterAgentStore,
        context,
        decisionIntervalTicks);
}

} // namespace Core