#pragma once

#include "character/player_profile.h"
#include "game/player_criminal_justice.h"
#include "game/player_wallet.h"
#include "game/player_world_state.h"
#include "sim/character_agent.h"
#include "world/chunk_store.h"
#include <cstdint>

namespace Core {

constexpr int32_t MAX_SOCIAL_ACTION_COUNT = 9;

enum class SocialActionKind : uint8_t {
    Visit = 0,
    ShareMeal = 1,
    SmallGift = 2,
    Favor = 3,
    Apologize = 4,
    IntroduceContact = 5,
    AskIntel = 6,
    Negotiate = 7,
    Warn = 8,
};

enum class SocialActionLockReason : uint8_t {
    None = 0,
    InactiveAgent = 1,
    PlayerIncarcerated = 2,
    AgentIncarcerated = 3,
    InsufficientOpinion = 4,
    InsufficientTrust = 5,
    DifferentBorough = 6,
    OnCooldown = 7,
    InsufficientCash = 8,
    RequiresNegativeOpinion = 9,
    RequiresRivalContext = 10,
    InsufficientNetwork = 11,
};

struct SocialActionDefinition {
    const char* id;
    const char* displayName;
    int32_t minOpinion;
    int32_t minTrust;
    bool requiresSameBorough;
    bool requiresNegativeOpinion;
    bool requiresRivalContext;
    float minNetworkAccess;
    int32_t cooldownTicks;
    int64_t cashCostCents;
    int32_t opinionDelta;
    int32_t trustDelta;
    int32_t respectDelta;
};

struct PlayerSocialActionStore {
    uint64_t lastActionTickByAgent[MAX_CHARACTER_AGENT_COUNT][MAX_SOCIAL_ACTION_COUNT]{};
};

void resetPlayerSocialActionStore(PlayerSocialActionStore& store);
int32_t getSocialActionCount();
const SocialActionDefinition* getSocialActionDefinition(int32_t actionIndex);
const SocialActionDefinition* getSocialActionDefinition(SocialActionKind actionKind);
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
    uint64_t tickCount);
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
    uint64_t tickCount);
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
    uint64_t tickCount);
const char* socialActionLockReasonToString(SocialActionLockReason reason);
const char* socialActionKindToLabel(SocialActionKind actionKind);

} // namespace Core
