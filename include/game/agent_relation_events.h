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
// Snitch pressure: 0 = will never talk, 100 = will always cooperate with police.
// Driven by low trust, low fear of player, poor opinion, and Survival motive.
constexpr int32_t AGENT_SNITCH_PRESSURE_THRESHOLD = 55;

void setAgentRelationEvent(CharacterAgentState& state, AgentRelationEventFlags flag);
bool hasAgentRelationEvent(const CharacterAgentState& state, AgentRelationEventFlags flag);
void clearAgentRelationEvent(CharacterAgentState& state, AgentRelationEventFlags flag);
void evaluateOpinionDeltaRelationEvents(
    CharacterAgentState& state,
    int32_t opinionDelta,
    bool wasCrewMember);
// Returns 0–100 likelihood of cooperation with police based on relationship state.
int32_t computeAgentSnitchPressure(const CharacterAgentState& state);
// Marks the agent as having snitched; only sets flag when snitch pressure meets threshold.
void markAgentSnitchedToPolice(CharacterAgentStore& store, int32_t agentIndex);
// Coerced intel extraction (kidnap/torture scenario): forces the snitch flag regardless
// of pressure, drops trust and opinion, raises fear. Returns true if agent was coerced.
bool tryCoerceAgentInformant(CharacterAgentStore& store, int32_t agentIndex);
void seedDefaultAgentRelationFlags(CharacterAgentStore& store, int32_t agentIndex);

} // namespace Core
