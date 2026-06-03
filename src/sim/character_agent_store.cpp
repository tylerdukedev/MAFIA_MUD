#include "sim/character_agent.h"
#include "character/character_social_network.h"
#include <algorithm>

namespace Core {

namespace {

constexpr AgentDefinition AGENT_DEFINITIONS[] = {
    {"landlord_schwartz", "Morris Schwartz", "Landlord", AgentMotive::Wealth, AgentEmotion::Suspicious, AgentPersonalityTrait::Paranoid, -5},
    {"beat_cop_hayes", "Officer Hayes", "Law", AgentMotive::Survival, AgentEmotion::Suspicious, AgentPersonalityTrait::Pragmatic, -20},
    {"union_delegate", "Frank Russo", "Labor", AgentMotive::Status, AgentEmotion::Calm, AgentPersonalityTrait::Charitable, 10},
    {"rival_torres", "Eddie Torres", "Rival", AgentMotive::Revenge, AgentEmotion::Angry, AgentPersonalityTrait::Ruthless, -40},
};

constexpr int32_t AGENT_DEFINITION_COUNT = static_cast<int32_t>(sizeof(AGENT_DEFINITIONS) / sizeof(AGENT_DEFINITIONS[0]));

void seedAgentState(CharacterAgentState& state, const AgentDefinition& definition) {
    state.opinionOfPlayer = definition.baselineOpinionOfPlayer;
    state.trust = std::clamp(40 + definition.baselineOpinionOfPlayer / 4, 0, 100);
    state.fear = std::clamp(20 - definition.baselineOpinionOfPlayer / 5, 0, 100);
    state.respect = std::clamp(35 + definition.baselineOpinionOfPlayer / 3, 0, 100);
    state.currentEmotion = definition.baselineEmotion;
    state.isActive = true;
}

} // namespace

int32_t getCharacterAgentDefinitionCount() {
    return AGENT_DEFINITION_COUNT;
}

const AgentDefinition* getCharacterAgentDefinition(int32_t agentIndex) {
    if (agentIndex < 0 || agentIndex >= AGENT_DEFINITION_COUNT) {
        return nullptr;
    }
    return &AGENT_DEFINITIONS[agentIndex];
}

void initializeCharacterAgentStore(CharacterAgentStore& store) {
    resetCharacterAgentStore(store);
    for (int32_t definitionIndex = 0; definitionIndex < AGENT_DEFINITION_COUNT; ++definitionIndex) {
        const AgentDefinition* definition = getCharacterAgentDefinition(definitionIndex);
        if (definition == nullptr) {
            continue;
        }
        const int32_t slotIndex = FIRST_COMMUNITY_AGENT_SLOT_INDEX + definitionIndex;
        seedAgentState(store.states[slotIndex], *definition);
    }
}

void resetCharacterAgentStore(CharacterAgentStore& store) {
    for (int32_t agentIndex = 0; agentIndex < MAX_CHARACTER_AGENT_COUNT; ++agentIndex) {
        store.states[agentIndex] = CharacterAgentState{};
    }
}

const CharacterAgentState* getCharacterAgentState(const CharacterAgentStore& store, int32_t agentIndex) {
    if (agentIndex < 0 || agentIndex >= MAX_CHARACTER_AGENT_COUNT) {
        return nullptr;
    }
    if (!store.states[agentIndex].isActive) {
        return nullptr;
    }
    return &store.states[agentIndex];
}

void adjustAgentOpinion(CharacterAgentStore& store, int32_t agentIndex, int32_t delta) {
    if (agentIndex < 0 || agentIndex >= MAX_CHARACTER_AGENT_COUNT) {
        return;
    }
    CharacterAgentState& state = store.states[agentIndex];
    if (!state.isActive) {
        return;
    }
    state.opinionOfPlayer = std::clamp(state.opinionOfPlayer + delta, AGENT_OPINION_MIN, AGENT_OPINION_MAX);
    state.trust = std::clamp(state.trust + delta / 3, 0, 100);
    state.respect = std::clamp(state.respect + delta / 4, 0, 100);
    if (delta < -8) {
        state.currentEmotion = AgentEmotion::Angry;
    } else     if (delta > 8) {
        state.currentEmotion = AgentEmotion::Grateful;
    }
}

bool tryGetAgentDisplayLabels(
    const CharacterAgentStore& store,
    int32_t agentIndex,
    const char*& outDisplayName,
    const char*& outRoleLabel) {
    if (agentIndex < 0 || agentIndex >= MAX_CHARACTER_AGENT_COUNT) {
        return false;
    }
    const CharacterAgentState& state = store.states[agentIndex];
    if (!state.isActive) {
        return false;
    }
    if (state.hasGeneratedIdentity) {
        outDisplayName = state.generatedDisplayName;
        outRoleLabel = state.generatedRoleLabel;
        return true;
    }
    const int32_t definitionIndex = agentIndex - FIRST_COMMUNITY_AGENT_SLOT_INDEX;
    const AgentDefinition* definition = getCharacterAgentDefinition(definitionIndex);
    if (definition == nullptr) {
        return false;
    }
    outDisplayName = definition->displayName;
    outRoleLabel = definition->roleLabel;
    return true;
}

} // namespace Core
