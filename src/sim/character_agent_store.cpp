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

const char* agentArchetypeToLabel(AgentArchetype archetype) {
    switch (archetype) {
        case AgentArchetype::Laborer: return "Laborer";
        case AgentArchetype::Dockworker: return "Dockworker";
        case AgentArchetype::Shopkeeper: return "Shopkeeper";
        case AgentArchetype::Clerk: return "Clerk";
        case AgentArchetype::Bartender: return "Bartender";
        case AgentArchetype::Cabbie: return "Cabbie";
        case AgentArchetype::Landlord: return "Landlord";
        case AgentArchetype::UnionMan: return "Union Man";
        case AgentArchetype::Priest: return "Priest";
        case AgentArchetype::Doctor: return "Doctor";
        case AgentArchetype::Patrolman: return "Patrolman";
        case AgentArchetype::Detective: return "Detective";
        case AgentArchetype::Politician: return "Politician";
        case AgentArchetype::Socialite: return "Socialite";
        case AgentArchetype::NumbersRunner: return "Numbers Runner";
        case AgentArchetype::Bootlegger: return "Bootlegger";
        case AgentArchetype::Enforcer: return "Enforcer";
        case AgentArchetype::Fence: return "Fence";
        case AgentArchetype::Loanshark: return "Loanshark";
        case AgentArchetype::Racketeer: return "Racketeer";
        case AgentArchetype::Vagrant: return "Vagrant";
        case AgentArchetype::Civilian:
        default: return "Civilian";
    }
}

const char* agentMotiveToLabel(AgentMotive motive) {
    switch (motive) {
        case AgentMotive::Survival: return "Survival";
        case AgentMotive::Wealth: return "Wealth";
        case AgentMotive::Status: return "Status";
        case AgentMotive::Loyalty: return "Loyalty";
        case AgentMotive::Revenge: return "Revenge";
        default: return "Unknown";
    }
}

const char* agentObjectiveToLabel(AgentObjective objective) {
    switch (objective) {
        case AgentObjective::EarnWage: return "Earn a wage";
        case AgentObjective::RestAtHome: return "Rest at home";
        case AgentObjective::Socialize: return "Socialize";
        case AgentObjective::SeekIncome: return "Scrape up cash";
        case AgentObjective::RunRacket: return "Run a racket";
        case AgentObjective::LayLow: return "Lay low";
        case AgentObjective::Patrol: return "Patrol the beat";
        case AgentObjective::SeekCare: return "Seek medical care";
        case AgentObjective::Settle:
        default: return "Settle in";
    }
}

const char* agentTraitToLabel(AgentPersonalityTrait trait) {
    switch (trait) {
        case AgentPersonalityTrait::Pragmatic: return "Pragmatic";
        case AgentPersonalityTrait::Proud: return "Proud";
        case AgentPersonalityTrait::Paranoid: return "Paranoid";
        case AgentPersonalityTrait::Charitable: return "Charitable";
        case AgentPersonalityTrait::Ruthless: return "Ruthless";
        default: return "Unknown";
    }
}

const char* agentEmotionToLabel(AgentEmotion emotion) {
    switch (emotion) {
        case AgentEmotion::Calm: return "Calm";
        case AgentEmotion::Anxious: return "Anxious";
        case AgentEmotion::Angry: return "Angry";
        case AgentEmotion::Grateful: return "Grateful";
        case AgentEmotion::Suspicious: return "Suspicious";
        default: return "Unknown";
    }
}

bool isCriminalArchetype(AgentArchetype archetype) {
    switch (archetype) {
        case AgentArchetype::NumbersRunner:
        case AgentArchetype::Bootlegger:
        case AgentArchetype::Enforcer:
        case AgentArchetype::Fence:
        case AgentArchetype::Loanshark:
        case AgentArchetype::Racketeer:
            return true;
        default:
            return false;
    }
}

bool isLawEnforcementArchetype(AgentArchetype archetype) {
    return archetype == AgentArchetype::Patrolman || archetype == AgentArchetype::Detective;
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
