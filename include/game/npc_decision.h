#pragma once

#include "sim/character_agent.h"
#include "game/game_calendar.h"
#include <cstdint>

namespace Core {

struct PropertyStore;

struct NPCDecisionContext {
    const GameCalendarStore* calendarStore;
    const PropertyStore* propertyStore;
    uint64_t tickCount;
    uint64_t worldSeed;
};

// Tick NPC decision making - called periodically (not every frame)
void tickNpcDecisions(
    CharacterAgentStore& agentStore,
    const NPCDecisionContext& context,
    int32_t ticksSinceLastDecision);

// Get display name for NPC's current activity
const char* getActivityDisplayLabel(AgentActivity activity);

} // namespace Core