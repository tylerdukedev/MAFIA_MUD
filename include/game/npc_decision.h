#pragma once

#include "sim/character_agent.h"
#include "sim/agent_relationship_graph.h"
#include "game/game_calendar.h"
#include <cstdint>

namespace Core {

struct PropertyStore;
class ChunkStore;
struct WorldConfig;

struct NPCDecisionContext {
    const GameCalendarStore* calendarStore = nullptr;
    const PropertyStore* propertyStore = nullptr;
    const ChunkStore* chunkStore = nullptr;
    const WorldConfig* worldConfig = nullptr;
    const AgentRelationshipGraph* relationshipGraph = nullptr;
    uint64_t tickCount = 0;
    uint64_t worldSeed = 0;
};

AgentObjective selectAgentObjective(
    const CharacterAgentState& agent,
    const GameCalendarStore& calendar,
    uint32_t decisionRoll);

// Tick NPC decision making - called periodically (not every frame)
void tickNpcDecisions(
    CharacterAgentStore& agentStore,
    const NPCDecisionContext& context,
    int32_t ticksSinceLastDecision);

// Applies monthly wages, racket income, rent and living costs to NPC cash on
// the same cadence as the player ledger. Bounded and allocation-free.
void tickNpcMonthlyLedger(
    CharacterAgentStore& agentStore,
    const PropertyStore& propertyStore,
    uint64_t tickCount,
    uint64_t worldSeed);

// Get display name for NPC's current activity
const char* getActivityDisplayLabel(AgentActivity activity);

} // namespace Core