#pragma once

#include "sim/character_agent.h"
#include <cstdint>

namespace Core {

enum class AgentRelationEventFlags : uint16_t {
    None = 0,
    BetrayedPlayer = 1U << 0,
    SnitchedToPolice = 1U << 1,
    OwesPlayerDebt = 1U << 2,
    MarkedRival = 1U << 3,
    InsultedPlayer = 1U << 4,
    CrewDefector = 1U << 5,
};

constexpr int32_t AGENT_BETRAYAL_OPINION_DROP_THRESHOLD = -12;
constexpr int32_t AGENT_BETRAYAL_TRUST_BEFORE_MIN = 45;

void setAgentRelationEvent(CharacterAgentState& state, AgentRelationEventFlags flag);
bool hasAgentRelationEvent(const CharacterAgentState& state, AgentRelationEventFlags flag);
void clearAgentRelationEvent(CharacterAgentState& state, AgentRelationEventFlags flag);
void evaluateOpinionDeltaRelationEvents(
    CharacterAgentState& state,
    int32_t opinionDelta,
    bool wasCrewMember);
void markAgentSnitchedToPolice(CharacterAgentStore& store, int32_t agentIndex);
void seedDefaultAgentRelationFlags(CharacterAgentStore& store, int32_t agentIndex);

} // namespace Core
