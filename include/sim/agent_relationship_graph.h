#pragma once

#include "sim/character_agent.h"
#include <cstdint>

namespace Core {

constexpr int32_t AGENT_MAX_TIES = 8;
constexpr int32_t AGENT_TIE_AFFINITY_MIN = -100;
constexpr int32_t AGENT_TIE_AFFINITY_MAX = 100;

// Relationship category between two NPCs. Player-facing relationships still
// live on CharacterAgentState; this graph models NPC-to-NPC bonds only.
enum class AgentTieKind : uint8_t {
    None = 0,
    Family = 1,
    Friend = 2,
    Coworker = 3,
    Associate = 4,
    Rival = 5,
};

struct AgentTie {
    int16_t targetAgentIndex = -1;
    int8_t affinity = 0;
    uint8_t kind = 0;
};

// SoA fixed-capacity adjacency: each agent keeps up to AGENT_MAX_TIES bonds.
struct AgentRelationshipGraph {
    AgentTie ties[MAX_CHARACTER_AGENT_COUNT][AGENT_MAX_TIES]{};
    uint8_t tieCount[MAX_CHARACTER_AGENT_COUNT]{};
};

void resetAgentRelationshipGraph(AgentRelationshipGraph& graph);
bool addAgentTie(AgentRelationshipGraph& graph, int32_t fromIndex, int32_t toIndex, AgentTieKind kind, int32_t affinity);
void linkAgentsBidirectional(AgentRelationshipGraph& graph, int32_t agentA, int32_t agentB, AgentTieKind kind, int32_t affinity);
int32_t getAgentTieCount(const AgentRelationshipGraph& graph, int32_t agentIndex);
const AgentTie* getAgentTie(const AgentRelationshipGraph& graph, int32_t agentIndex, int32_t tieIndex);
void adjustAgentTieAffinity(AgentRelationshipGraph& graph, int32_t fromIndex, int32_t toIndex, int32_t delta);
const char* agentTieKindToLabel(AgentTieKind kind);

} // namespace Core
