#include "game/agent_relation_events.h"
#include "character/character_social_network.h"
#include "game/player_organization.h"

namespace Core {

void setAgentRelationEvent(CharacterAgentState& state, AgentRelationEventFlags flag) {
    state.relationEventFlags |= static_cast<uint16_t>(flag);
}

bool hasAgentRelationEvent(const CharacterAgentState& state, AgentRelationEventFlags flag) {
    return (state.relationEventFlags & static_cast<uint16_t>(flag)) != 0U;
}

void clearAgentRelationEvent(CharacterAgentState& state, AgentRelationEventFlags flag) {
    state.relationEventFlags &= static_cast<uint16_t>(~static_cast<uint16_t>(flag));
}

void evaluateOpinionDeltaRelationEvents(
    CharacterAgentState& state,
    int32_t opinionDelta,
    bool wasCrewMember) {
    if (opinionDelta <= AGENT_BETRAYAL_OPINION_DROP_THRESHOLD && state.trust >= AGENT_BETRAYAL_TRUST_BEFORE_MIN) {
        setAgentRelationEvent(state, AgentRelationEventFlags::BetrayedPlayer);
    }
    if (opinionDelta <= -6) {
        setAgentRelationEvent(state, AgentRelationEventFlags::InsultedPlayer);
    }
    if (wasCrewMember && opinionDelta <= -10) {
        setAgentRelationEvent(state, AgentRelationEventFlags::CrewDefector);
    }
}

void markAgentSnitchedToPolice(CharacterAgentStore& store, int32_t agentIndex) {
    if (agentIndex < 0 || agentIndex >= MAX_CHARACTER_AGENT_COUNT) {
        return;
    }
    CharacterAgentState& state = store.states[agentIndex];
    if (!state.isActive) {
        return;
    }
    setAgentRelationEvent(state, AgentRelationEventFlags::SnitchedToPolice);
}

void seedDefaultAgentRelationFlags(CharacterAgentStore& store, int32_t agentIndex) {
    if (agentIndex == RIVAL_AGENT_SLOT_INDEX && store.states[agentIndex].isActive) {
        setAgentRelationEvent(store.states[agentIndex], AgentRelationEventFlags::MarkedRival);
    }
}

} // namespace Core
