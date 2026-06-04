#include "sim/character_agent.h"
#include "game/agent_relation_events.h"
#include "character/character_social_network.h"
#include <algorithm>
#include <cstring>

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
    deriveRelationshipStatsFromOpinion(state);
    state.currentEmotion = definition.baselineEmotion;
    state.isActive = true;
}

} // namespace

void deriveRelationshipStatsFromOpinion(CharacterAgentState& state) {
    const int32_t opinion = state.opinionOfPlayer;
    if (opinion >= 0) {
        state.trust = std::clamp(28 + opinion / 2, 0, 100);
        state.respect = std::clamp(20 + opinion / 2, 0, 100);
        state.fear = std::clamp(12 - opinion / 8, 0, 100);
        return;
    }
    const int32_t hostility = -opinion;
    state.trust = std::clamp(24 - hostility / 2, 0, 100);
    state.respect = std::clamp(30 - hostility / 3, 0, 100);
    state.fear = std::clamp(10 + hostility / 4, 0, 100);
}

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
        if (std::strcmp(definition->id, "landlord_schwartz") == 0) {
            store.states[slotIndex].isActive = false;
            continue;
        }
        seedAgentState(store.states[slotIndex], *definition);
        seedDefaultAgentRelationFlags(store, slotIndex);
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
    deriveRelationshipStatsFromOpinion(state);
    evaluateOpinionDeltaRelationEvents(state, delta, false);
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

void setAgentPosition(CharacterAgentState& state, int32_t tileX, int32_t tileY) {
    state.currentTileX = tileX;
    state.currentTileY = tileY;
}

void setAgentActivity(CharacterAgentState& state, AgentActivity activity, uint64_t tickCount) {
    state.currentActivity = activity;
    state.activityStartTick = tickCount;
    updateAgentMapVisibility(state);
}

bool isAgentVisibleOnMap(const CharacterAgentState& state) {
    if (!state.isActive) {
        return false;
    }
    return state.isVisibleOnMap;
}

void updateAgentMapVisibility(CharacterAgentState& state) {
    switch (state.currentActivity) {
        case AgentActivity::Idle:
        case AgentActivity::Traveling:
            state.isVisibleOnMap = true;
            break;
        case AgentActivity::AtHome:
        case AgentActivity::AtWork:
        case AgentActivity::InBuilding:
        case AgentActivity::Abroad:
        case AgentActivity::Incarcerated:
            state.isVisibleOnMap = false;
            break;
        default:
            state.isVisibleOnMap = false;
            break;
    }
}

} // namespace Core
