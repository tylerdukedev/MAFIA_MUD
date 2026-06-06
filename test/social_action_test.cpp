#include <catch2/catch_test_macros.hpp>
#include <cstring>
#include "character/profile_builder.h"
#include "game/player_criminal_justice.h"
#include "game/player_wallet.h"
#include "game/player_world_state.h"
#include "game/social_action_catalog.h"
#include "character/character_social_network.h"
#include "sim/character_agent.h"
#include "world/chunk_store.h"
#include "world/world_config.h"

namespace Core {

TEST_CASE("Social action catalog exposes nine rapport actions", "[social_action]") {
    REQUIRE(getSocialActionCount() == 9);
    const SocialActionDefinition* visit = getSocialActionDefinition(SocialActionKind::Visit);
    REQUIRE(visit != nullptr);
    REQUIRE(visit->displayName != nullptr);
    REQUIRE(std::strcmp(visit->displayName, "Visit") == 0);
}

TEST_CASE("Visit raises opinion when prerequisites pass", "[social_action]") {
    CharacterDraft draft{};
    const PlayerProfile profile = buildPlayerProfile(draft);
    PlayerSocialActionStore socialStore{};
    PlayerWallet wallet{};
    wallet.cashCents = 5000;
    PlayerCriminalJusticeStore justiceStore{};
    PlayerWorldState worldState{};
    worldState.currentRegionId = static_cast<uint8_t>(RegionId::Manhattan);
    CharacterAgentStore agents{};
    initializeCharacterAgentStore(agents);
    const int32_t unionDelegateIndex = FIRST_COMMUNITY_AGENT_SLOT_INDEX + 2;
    const int32_t opinionBefore = agents.states[unionDelegateIndex].opinionOfPlayer;
    const WorldConfig worldConfig{};
    ChunkStore chunkStore{worldConfig};
    const int32_t visitIndex = static_cast<int32_t>(SocialActionKind::Visit);
    REQUIRE(canExecuteSocialAction(
        socialStore, justiceStore, wallet, worldState, profile, agents, chunkStore, unionDelegateIndex, visitIndex, 100ULL));
    REQUIRE(tryExecuteSocialAction(
        socialStore, wallet, agents, justiceStore, worldState, profile, chunkStore, unionDelegateIndex, visitIndex, 100ULL));
    REQUIRE(agents.states[unionDelegateIndex].opinionOfPlayer == opinionBefore + 2);
}

TEST_CASE("Small gift blocks without cash", "[social_action]") {
    CharacterDraft draft{};
    const PlayerProfile profile = buildPlayerProfile(draft);
    PlayerSocialActionStore socialStore{};
    PlayerWallet wallet{};
    wallet.cashCents = 100;
    PlayerCriminalJusticeStore justiceStore{};
    PlayerWorldState worldState{};
    CharacterAgentStore agents{};
    initializeCharacterAgentStore(agents);
    CharacterAgentState& agent = agents.states[FRIEND_AGENT_SLOT_INDEX];
    agent.isActive = true;
    agent.opinionOfPlayer = 5;
    agent.trust = 30;
    deriveRelationshipStatsFromOpinion(agent);
    const WorldConfig worldConfig{};
    ChunkStore chunkStore{worldConfig};
    const int32_t giftIndex = static_cast<int32_t>(SocialActionKind::SmallGift);
    const SocialActionLockReason lockReason = evaluateSocialActionLock(
        socialStore, justiceStore, wallet, worldState, profile, agents, chunkStore, FRIEND_AGENT_SLOT_INDEX, giftIndex, 50ULL);
    REQUIRE(lockReason == SocialActionLockReason::InsufficientCash);
}

TEST_CASE("Apologize requires negative opinion", "[social_action]") {
    CharacterDraft draft{};
    const PlayerProfile profile = buildPlayerProfile(draft);
    PlayerSocialActionStore socialStore{};
    PlayerWallet wallet{};
    PlayerCriminalJusticeStore justiceStore{};
    PlayerWorldState worldState{};
    CharacterAgentStore agents{};
    initializeCharacterAgentStore(agents);
    CharacterAgentState& agent = agents.states[RIVAL_AGENT_SLOT_INDEX];
    agent.isActive = true;
    agent.opinionOfPlayer = 15;
    agent.trust = 50;
    deriveRelationshipStatsFromOpinion(agent);
    const WorldConfig worldConfig{};
    ChunkStore chunkStore{worldConfig};
    const int32_t apologizeIndex = static_cast<int32_t>(SocialActionKind::Apologize);
    const SocialActionLockReason lockReason = evaluateSocialActionLock(
        socialStore, justiceStore, wallet, worldState, profile, agents, chunkStore, RIVAL_AGENT_SLOT_INDEX, apologizeIndex, 80ULL);
    REQUIRE(lockReason == SocialActionLockReason::RequiresNegativeOpinion);
}

TEST_CASE("Social action cooldown blocks immediate repeat", "[social_action]") {
    CharacterDraft draft{};
    const PlayerProfile profile = buildPlayerProfile(draft);
    PlayerSocialActionStore socialStore{};
    PlayerWallet wallet{};
    wallet.cashCents = 5000;
    PlayerCriminalJusticeStore justiceStore{};
    PlayerWorldState worldState{};
    CharacterAgentStore agents{};
    initializeCharacterAgentStore(agents);
    CharacterAgentState& agent = agents.states[FRIEND_AGENT_SLOT_INDEX];
    agent.isActive = true;
    agent.opinionOfPlayer = 20;
    agent.trust = 45;
    deriveRelationshipStatsFromOpinion(agent);
    const WorldConfig worldConfig{};
    ChunkStore chunkStore{worldConfig};
    const int32_t visitIndex = static_cast<int32_t>(SocialActionKind::Visit);
    REQUIRE(tryExecuteSocialAction(
        socialStore, wallet, agents, justiceStore, worldState, profile, chunkStore, FRIEND_AGENT_SLOT_INDEX, visitIndex, 200ULL));
    const SocialActionLockReason lockReason = evaluateSocialActionLock(
        socialStore, justiceStore, wallet, worldState, profile, agents, chunkStore, FRIEND_AGENT_SLOT_INDEX, visitIndex, 250ULL);
    REQUIRE(lockReason == SocialActionLockReason::OnCooldown);
}

} // namespace Core
