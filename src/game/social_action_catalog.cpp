#include "game/social_action_catalog.h"
#include "game/agent_relation_events.h"
#include "game/player_operations.h"
#include <algorithm>

namespace Core {

namespace {

constexpr SocialActionDefinition SOCIAL_ACTION_DEFINITIONS[] = {
    {"visit", "Visit", -50, 0, true, false, false, 0.0f, 80, 0, 2, 1, 1},
    {"share_meal", "Share a meal", 0, 20, true, false, false, 0.0f, 160, 800, 4, 3, 2},
    {"small_gift", "Small gift", -10, 15, true, false, false, 0.0f, 120, 500, 3, 2, 2},
    {"favor", "Do a favor", 10, 30, true, false, false, 0.0f, 200, 0, 5, 4, 3},
    {"apologize", "Apologize", -100, 0, false, true, false, 0.0f, 240, 0, 6, 3, 2},
    {"introduce", "Introduce contact", 25, 40, true, false, false, 0.15f, 320, 0, 3, 5, 4},
    {"ask_intel", "Ask for intel", 15, 35, true, false, false, 0.0f, 240, 200, -1, -2, 0},
    {"negotiate", "Negotiate truce", -30, 20, true, false, true, 0.0f, 400, 1000, 4, 3, 5},
    {"warn", "Warn them", -20, 25, true, false, false, 0.0f, 160, 0, -2, 1, 3},
};

constexpr int32_t SOCIAL_ACTION_DEFINITION_COUNT = static_cast<int32_t>(sizeof(SOCIAL_ACTION_DEFINITIONS) / sizeof(SOCIAL_ACTION_DEFINITIONS[0]));

bool isAgentInSameBoroughAsPlayer(
    const CharacterAgentState& agentState,
    const PlayerWorldState& worldState,
    const ChunkStore& chunkStore) {
    if (agentState.currentTileX < 0 || agentState.currentTileY < 0) {
        return true;
    }
    const WorldCoord agentCoord{agentState.currentTileX, agentState.currentTileY};
    if (!chunkStore.hasTileAt(agentCoord)) {
        return true;
    }
    const RegionId agentRegion = chunkStore.getRegionAt(agentCoord);
    const RegionId playerRegion = static_cast<RegionId>(worldState.currentRegionId);
    return agentRegion == playerRegion;
}

} // namespace

void resetPlayerSocialActionStore(PlayerSocialActionStore& store) {
    for (int32_t agentIndex = 0; agentIndex < MAX_CHARACTER_AGENT_COUNT; ++agentIndex) {
        for (int32_t actionIndex = 0; actionIndex < MAX_SOCIAL_ACTION_COUNT; ++actionIndex) {
            store.lastActionTickByAgent[agentIndex][actionIndex] = 0ULL;
        }
    }
}

int32_t getSocialActionCount() {
    return SOCIAL_ACTION_DEFINITION_COUNT;
}

const SocialActionDefinition* getSocialActionDefinition(int32_t actionIndex) {
    if (actionIndex < 0 || actionIndex >= SOCIAL_ACTION_DEFINITION_COUNT) {
        return nullptr;
    }
    return &SOCIAL_ACTION_DEFINITIONS[actionIndex];
}

const SocialActionDefinition* getSocialActionDefinition(SocialActionKind actionKind) {
    return getSocialActionDefinition(static_cast<int32_t>(actionKind));
}

SocialActionLockReason evaluateSocialActionLock(
    const PlayerSocialActionStore& socialStore,
    const PlayerCriminalJusticeStore& justiceStore,
    const PlayerWallet& wallet,
    const PlayerWorldState& worldState,
    const PlayerProfile& profile,
    const CharacterAgentStore& agentStore,
    const ChunkStore& chunkStore,
    int32_t agentIndex,
    int32_t actionIndex,
    uint64_t tickCount) {
    const SocialActionDefinition* action = getSocialActionDefinition(actionIndex);
    if (action == nullptr) {
        return SocialActionLockReason::InactiveAgent;
    }
    const CharacterAgentState* state = getCharacterAgentState(agentStore, agentIndex);
    if (state == nullptr) {
        return SocialActionLockReason::InactiveAgent;
    }
    if (isPlayerFullyIncarcerated(justiceStore)) {
        return SocialActionLockReason::PlayerIncarcerated;
    }
    if (state->currentActivity == AgentActivity::Incarcerated) {
        return SocialActionLockReason::AgentIncarcerated;
    }
    if (action->requiresRivalContext) {
        const bool hasRivalContext = hasAgentRelationEvent(*state, AgentRelationEventFlags::MarkedRival)
            || state->opinionOfPlayer >= action->minOpinion;
        if (!hasRivalContext) {
            return SocialActionLockReason::RequiresRivalContext;
        }
    } else if (state->opinionOfPlayer < action->minOpinion) {
        return SocialActionLockReason::InsufficientOpinion;
    }
    if (state->trust < action->minTrust) {
        return SocialActionLockReason::InsufficientTrust;
    }
    if (action->requiresNegativeOpinion && state->opinionOfPlayer >= 0) {
        return SocialActionLockReason::RequiresNegativeOpinion;
    }
    if (action->minNetworkAccess > 0.0f && getNetworkAccessScore(profile) < action->minNetworkAccess) {
        return SocialActionLockReason::InsufficientNetwork;
    }
    if (action->requiresSameBorough && !isAgentInSameBoroughAsPlayer(*state, worldState, chunkStore)) {
        return SocialActionLockReason::DifferentBorough;
    }
    if (action->cashCostCents > 0 && !canAffordCash(wallet, action->cashCostCents)) {
        return SocialActionLockReason::InsufficientCash;
    }
    const uint64_t lastActionTick = socialStore.lastActionTickByAgent[agentIndex][actionIndex];
    if (lastActionTick != 0ULL && tickCount < lastActionTick + static_cast<uint64_t>(action->cooldownTicks)) {
        return SocialActionLockReason::OnCooldown;
    }
    return SocialActionLockReason::None;
}

bool canExecuteSocialAction(
    const PlayerSocialActionStore& socialStore,
    const PlayerCriminalJusticeStore& justiceStore,
    const PlayerWallet& wallet,
    const PlayerWorldState& worldState,
    const PlayerProfile& profile,
    const CharacterAgentStore& agentStore,
    const ChunkStore& chunkStore,
    int32_t agentIndex,
    int32_t actionIndex,
    uint64_t tickCount) {
    return evaluateSocialActionLock(
               socialStore,
               justiceStore,
               wallet,
               worldState,
               profile,
               agentStore,
               chunkStore,
               agentIndex,
               actionIndex,
               tickCount)
        == SocialActionLockReason::None;
}

bool tryExecuteSocialAction(
    PlayerSocialActionStore& socialStore,
    PlayerWallet& wallet,
    CharacterAgentStore& agentStore,
    const PlayerCriminalJusticeStore& justiceStore,
    const PlayerWorldState& worldState,
    const PlayerProfile& profile,
    const ChunkStore& chunkStore,
    int32_t agentIndex,
    int32_t actionIndex,
    uint64_t tickCount) {
    if (!canExecuteSocialAction(
            socialStore,
            justiceStore,
            wallet,
            worldState,
            profile,
            agentStore,
            chunkStore,
            agentIndex,
            actionIndex,
            tickCount)) {
        return false;
    }
    const SocialActionDefinition* action = getSocialActionDefinition(actionIndex);
    if (action == nullptr) {
        return false;
    }
    if (action->cashCostCents > 0 && !tryDebitCash(wallet, action->cashCostCents)) {
        return false;
    }
    if (action->opinionDelta != 0) {
        adjustAgentOpinion(agentStore, agentIndex, action->opinionDelta);
    }
    CharacterAgentState& state = agentStore.states[agentIndex];
    if (action->trustDelta != 0) {
        state.trust = std::clamp(state.trust + action->trustDelta, 0, 100);
    }
    if (action->respectDelta != 0) {
        state.respect = std::clamp(state.respect + action->respectDelta, 0, 100);
    }
    if (action->opinionDelta >= 4) {
        state.currentEmotion = AgentEmotion::Grateful;
    } else if (action->opinionDelta <= -2) {
        state.currentEmotion = AgentEmotion::Suspicious;
    }
    if (actionIndex == static_cast<int32_t>(SocialActionKind::Apologize)) {
        clearAgentRelationEvent(state, AgentRelationEventFlags::InsultedPlayer);
    }
    if (actionIndex == static_cast<int32_t>(SocialActionKind::Negotiate)
        && state.opinionOfPlayer >= -10
        && hasAgentRelationEvent(state, AgentRelationEventFlags::MarkedRival)) {
        clearAgentRelationEvent(state, AgentRelationEventFlags::MarkedRival);
    }
    socialStore.lastActionTickByAgent[agentIndex][actionIndex] = tickCount;
    return true;
}

const char* socialActionLockReasonToString(SocialActionLockReason reason) {
    switch (reason) {
    case SocialActionLockReason::None:
        return "Available";
    case SocialActionLockReason::InactiveAgent:
        return "Contact unavailable";
    case SocialActionLockReason::PlayerIncarcerated:
        return "You are incarcerated";
    case SocialActionLockReason::AgentIncarcerated:
        return "Contact is incarcerated";
    case SocialActionLockReason::InsufficientOpinion:
        return "Opinion too low";
    case SocialActionLockReason::InsufficientTrust:
        return "Trust too low";
    case SocialActionLockReason::DifferentBorough:
        return "Must be in same borough";
    case SocialActionLockReason::OnCooldown:
        return "On cooldown";
    case SocialActionLockReason::InsufficientCash:
        return "Insufficient cash";
    case SocialActionLockReason::RequiresNegativeOpinion:
        return "Only when opinion is negative";
    case SocialActionLockReason::RequiresRivalContext:
        return "Needs rival or thawing opinion";
    case SocialActionLockReason::InsufficientNetwork:
        return "Requires higher network access";
    default:
        return "Unavailable";
    }
}

const char* socialActionKindToLabel(SocialActionKind actionKind) {
    const SocialActionDefinition* action = getSocialActionDefinition(actionKind);
    if (action == nullptr) {
        return "Social action";
    }
    return action->displayName;
}

} // namespace Core
