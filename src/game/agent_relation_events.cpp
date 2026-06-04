#include "game/agent_relation_events.h"
#include "character/character_social_network.h"
#include "game/player_organization.h"
#include <algorithm>

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

int32_t computeAgentSnitchPressure(const CharacterAgentState& state) {
    // Base pressure from inverse trust: low trust = high pressure.
    int32_t pressure = (100 - std::clamp(state.trust, 0, 100)) / 2;
    // Low fear of the player removes the reason to stay quiet.
    if (state.fear < 20) {
        pressure += 20;
    } else if (state.fear < 40) {
        pressure += 8;
    }
    // Negative opinion means they dislike the player enough to cooperate with police.
    if (state.opinionOfPlayer < -20) {
        pressure += 18;
    } else if (state.opinionOfPlayer < 0) {
        pressure += 8;
    }
    // Survival motive: self-preservation overrides loyalty when cornered.
    if (state.generatedMotive == AgentMotive::Survival) {
        pressure += 14;
    }
    // Loyalty motive strongly resists cooperating with police.
    if (state.generatedMotive == AgentMotive::Loyalty) {
        pressure -= 18;
    }
    // Paranoid trait: suspects the consequences of staying silent are worse.
    if (state.generatedTrait == AgentPersonalityTrait::Paranoid) {
        pressure += 10;
    }
    return std::clamp(pressure, 0, 100);
}

void markAgentSnitchedToPolice(CharacterAgentStore& store, int32_t agentIndex) {
    if (agentIndex < 0 || agentIndex >= MAX_CHARACTER_AGENT_COUNT) {
        return;
    }
    CharacterAgentState& state = store.states[agentIndex];
    if (!state.isActive) {
        return;
    }
    // Only contacts with enough motivation to cooperate with police will snitch.
    if (computeAgentSnitchPressure(state) < AGENT_SNITCH_PRESSURE_THRESHOLD) {
        return;
    }
    setAgentRelationEvent(state, AgentRelationEventFlags::SnitchedToPolice);
    // Snitching signals a break in loyalty — drop trust and opinion.
    state.trust = std::max(0, state.trust - 20);
    state.opinionOfPlayer = std::max(AGENT_OPINION_MIN, state.opinionOfPlayer - 12);
}

bool tryCoerceAgentInformant(CharacterAgentStore& store, int32_t agentIndex) {
    if (agentIndex < 0 || agentIndex >= MAX_CHARACTER_AGENT_COUNT) {
        return false;
    }
    CharacterAgentState& state = store.states[agentIndex];
    if (!state.isActive) {
        return false;
    }
    setAgentRelationEvent(state, AgentRelationEventFlags::SnitchedToPolice);
    // Coercion destroys the relationship: trust collapses, fear spikes, opinion craters.
    state.trust = std::max(0, state.trust - 40);
    state.fear = std::min(100, state.fear + 35);
    state.opinionOfPlayer = std::max(AGENT_OPINION_MIN, state.opinionOfPlayer - 25);
    return true;
}

void seedDefaultAgentRelationFlags(CharacterAgentStore& store, int32_t agentIndex) {
    if (agentIndex == RIVAL_AGENT_SLOT_INDEX && store.states[agentIndex].isActive) {
        setAgentRelationEvent(store.states[agentIndex], AgentRelationEventFlags::MarkedRival);
    }
}

} // namespace Core
