#include "sim/agent_relationship_graph.h"
#include <algorithm>

namespace Core {

namespace {

bool isValidAgentIndex(int32_t agentIndex) {
    return agentIndex >= 0 && agentIndex < MAX_CHARACTER_AGENT_COUNT;
}

int32_t findTieSlot(const AgentRelationshipGraph& graph, int32_t fromIndex, int32_t toIndex) {
    const int32_t count = graph.tieCount[fromIndex];
    for (int32_t tieIndex = 0; tieIndex < count; ++tieIndex) {
        if (graph.ties[fromIndex][tieIndex].targetAgentIndex == static_cast<int16_t>(toIndex)) {
            return tieIndex;
        }
    }
    return -1;
}

} // namespace

void resetAgentRelationshipGraph(AgentRelationshipGraph& graph) {
    for (int32_t agentIndex = 0; agentIndex < MAX_CHARACTER_AGENT_COUNT; ++agentIndex) {
        graph.tieCount[agentIndex] = 0;
        for (int32_t tieIndex = 0; tieIndex < AGENT_MAX_TIES; ++tieIndex) {
            graph.ties[agentIndex][tieIndex] = AgentTie{};
        }
    }
}

bool addAgentTie(AgentRelationshipGraph& graph, int32_t fromIndex, int32_t toIndex, AgentTieKind kind, int32_t affinity) {
    if (!isValidAgentIndex(fromIndex) || !isValidAgentIndex(toIndex) || fromIndex == toIndex) {
        return false;
    }
    const int8_t clampedAffinity = static_cast<int8_t>(std::clamp(affinity, AGENT_TIE_AFFINITY_MIN, AGENT_TIE_AFFINITY_MAX));
    const int32_t existingSlot = findTieSlot(graph, fromIndex, toIndex);
    if (existingSlot >= 0) {
        graph.ties[fromIndex][existingSlot].kind = static_cast<uint8_t>(kind);
        graph.ties[fromIndex][existingSlot].affinity = clampedAffinity;
        return true;
    }
    if (graph.tieCount[fromIndex] >= AGENT_MAX_TIES) {
        return false;
    }
    const int32_t slot = graph.tieCount[fromIndex];
    graph.ties[fromIndex][slot].targetAgentIndex = static_cast<int16_t>(toIndex);
    graph.ties[fromIndex][slot].kind = static_cast<uint8_t>(kind);
    graph.ties[fromIndex][slot].affinity = clampedAffinity;
    graph.tieCount[fromIndex] += 1;
    return true;
}

void linkAgentsBidirectional(AgentRelationshipGraph& graph, int32_t agentA, int32_t agentB, AgentTieKind kind, int32_t affinity) {
    addAgentTie(graph, agentA, agentB, kind, affinity);
    addAgentTie(graph, agentB, agentA, kind, affinity);
}

int32_t getAgentTieCount(const AgentRelationshipGraph& graph, int32_t agentIndex) {
    if (!isValidAgentIndex(agentIndex)) {
        return 0;
    }
    return static_cast<int32_t>(graph.tieCount[agentIndex]);
}

const AgentTie* getAgentTie(const AgentRelationshipGraph& graph, int32_t agentIndex, int32_t tieIndex) {
    if (!isValidAgentIndex(agentIndex)) {
        return nullptr;
    }
    if (tieIndex < 0 || tieIndex >= static_cast<int32_t>(graph.tieCount[agentIndex])) {
        return nullptr;
    }
    return &graph.ties[agentIndex][tieIndex];
}

void adjustAgentTieAffinity(AgentRelationshipGraph& graph, int32_t fromIndex, int32_t toIndex, int32_t delta) {
    if (!isValidAgentIndex(fromIndex) || !isValidAgentIndex(toIndex)) {
        return;
    }
    const int32_t slot = findTieSlot(graph, fromIndex, toIndex);
    if (slot < 0) {
        return;
    }
    const int32_t updated = static_cast<int32_t>(graph.ties[fromIndex][slot].affinity) + delta;
    graph.ties[fromIndex][slot].affinity = static_cast<int8_t>(std::clamp(updated, AGENT_TIE_AFFINITY_MIN, AGENT_TIE_AFFINITY_MAX));
}

const char* agentTieKindToLabel(AgentTieKind kind) {
    switch (kind) {
        case AgentTieKind::Family: return "Family";
        case AgentTieKind::Friend: return "Friend";
        case AgentTieKind::Coworker: return "Coworker";
        case AgentTieKind::Associate: return "Associate";
        case AgentTieKind::Rival: return "Rival";
        case AgentTieKind::None:
        default: return "Acquaintance";
    }
}

} // namespace Core
