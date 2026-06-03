#include "character/character_social_network.h"
#include "character/character_name_pools.h"
#include "sim/character_agent.h"
#include "character/character_types.h"
#include "utils/seed_hash.h"
#include <algorithm>
#include <cstdio>
#include <cstring>

namespace Core {

namespace {

constexpr int32_t FAMILY_CHANCE_IMMIGRANT_PERCENT = 32;
constexpr int32_t FAMILY_CHANCE_FIRST_GEN_PERCENT = 52;
constexpr int32_t FAMILY_CHANCE_SECOND_GEN_PERCENT = 68;
constexpr int32_t FAMILY_CHANCE_THIRD_GEN_PERCENT = 74;
constexpr int32_t FRIEND_BASE_CHANCE_PERCENT = 38;
constexpr int32_t FRIEND_STREET_BONUS_PERCENT = 12;
constexpr int32_t FRIEND_ORGANIZER_BONUS_PERCENT = 10;
constexpr int32_t FRIEND_SECOND_GEN_BONUS_PERCENT = 8;
constexpr int32_t FRIEND_IMMIGRANT_PENALTY_PERCENT = 14;

constexpr const char* FAMILY_RELATION_LABELS[] = {"Uncle", "Aunt", "Cousin", "Older brother", "Older sister", "In-law"};
constexpr const char* FRIEND_RELATION_LABELS[] = {"Childhood friend", "Tenement neighbor", "Work buddy", "Boarding roommate"};

int32_t rollPercentChance(uint64_t seed, int32_t salt, int32_t thresholdPercent) {
    const uint32_t roll = Utils::hashSeedMix(seed, salt, 0x534F434C) % 100U;
    return static_cast<int32_t>(roll) < thresholdPercent ? 1 : 0;
}

int32_t computeFamilyChancePercent(const CharacterDraft& draft) {
    if (draft.generationId == GenerationId::Immigrant) {
        return FAMILY_CHANCE_IMMIGRANT_PERCENT;
    }
    if (draft.generationId == GenerationId::FirstGeneration) {
        return FAMILY_CHANCE_FIRST_GEN_PERCENT;
    }
    if (draft.generationId == GenerationId::SecondGeneration) {
        return FAMILY_CHANCE_SECOND_GEN_PERCENT;
    }
    if (draft.generationId == GenerationId::ThirdGenerationPlus) {
        return FAMILY_CHANCE_THIRD_GEN_PERCENT;
    }
    return FAMILY_CHANCE_FIRST_GEN_PERCENT;
}

int32_t computeFriendChancePercent(const CharacterDraft& draft) {
    int32_t chance = FRIEND_BASE_CHANCE_PERCENT;
    if (draft.backgroundId == BackgroundId::StreetHustler) {
        chance += FRIEND_STREET_BONUS_PERCENT;
    }
    if (draft.backgroundId == BackgroundId::NeighborhoodOrganizer) {
        chance += FRIEND_ORGANIZER_BONUS_PERCENT;
    }
    if (draft.generationId == GenerationId::SecondGeneration) {
        chance += FRIEND_SECOND_GEN_BONUS_PERCENT;
    }
    if (draft.generationId == GenerationId::Immigrant) {
        chance -= FRIEND_IMMIGRANT_PENALTY_PERCENT;
    }
    return std::clamp(chance, 18, 72);
}

void pickFromPool(const char* const* pool, int32_t poolCount, uint64_t seed, int32_t salt, char* outBuffer, size_t bufferSize) {
    pickCharacterNameFromPool(outBuffer, bufferSize, pool, poolCount, seed, salt);
}

AgentMotive rollMotive(uint64_t seed, int32_t salt) {
    const uint32_t roll = Utils::hashSeedMix(seed, salt, 0x4D4F5456) % 5U;
    return static_cast<AgentMotive>(roll);
}

AgentPersonalityTrait rollTrait(uint64_t seed, int32_t salt) {
    const uint32_t roll = Utils::hashSeedMix(seed, salt, 0x54524149) % 5U;
    return static_cast<AgentPersonalityTrait>(roll);
}

AgentEmotion rollEmotion(uint64_t seed, int32_t salt) {
    const uint32_t roll = Utils::hashSeedMix(seed, salt, 0x454D4F54) % 5U;
    return static_cast<AgentEmotion>(roll);
}

int32_t rollBaselineOpinion(uint64_t seed, int32_t salt, int32_t minValue, int32_t maxValue) {
    const uint32_t span = static_cast<uint32_t>(maxValue - minValue + 1);
    const uint32_t roll = Utils::hashSeedMix(seed, salt, 0x4F50494E) % span;
    return minValue + static_cast<int32_t>(roll);
}

void seedGeneratedAgent(
    CharacterAgentState& state,
    const char* displayName,
    const char* roleLabel,
    AgentMotive motive,
    AgentPersonalityTrait trait,
    AgentEmotion emotion,
    int32_t baselineOpinion) {
    std::strncpy(state.generatedDisplayName, displayName, sizeof(state.generatedDisplayName) - 1);
    std::strncpy(state.generatedRoleLabel, roleLabel, sizeof(state.generatedRoleLabel) - 1);
    state.generatedMotive = motive;
    state.generatedTrait = trait;
    state.hasGeneratedIdentity = true;
    state.opinionOfPlayer = baselineOpinion;
    deriveRelationshipStatsFromOpinion(state);
    state.currentEmotion = emotion;
    state.isActive = true;
}

void spawnFamilyContact(const CharacterDraft& draft, CharacterAgentStore& store) {
    char displayName[32]{};
    buildRandomCharacterName(displayName, sizeof(displayName), draft.heritageId, draft.nationalityId, draft.characterRollSeed, 0x46414D31);
    char relation[24]{};
    pickFromPool(FAMILY_RELATION_LABELS, static_cast<int32_t>(sizeof(FAMILY_RELATION_LABELS) / sizeof(FAMILY_RELATION_LABELS[0])), draft.characterRollSeed, 0x46414D32, relation, sizeof(relation));
    const int32_t opinion = rollBaselineOpinion(draft.characterRollSeed, 0x46414D33, 18, 42);
    seedGeneratedAgent(
        store.states[FAMILY_AGENT_SLOT_INDEX],
        displayName,
        relation,
        rollMotive(draft.characterRollSeed, 0x46414D34),
        rollTrait(draft.characterRollSeed, 0x46414D35),
        rollEmotion(draft.characterRollSeed, 0x46414D36),
        opinion);
}

void spawnFriendContact(const CharacterDraft& draft, CharacterAgentStore& store) {
    char displayName[32]{};
    buildRandomCharacterName(displayName, sizeof(displayName), draft.heritageId, draft.nationalityId, draft.characterRollSeed, 0x46524931);
    char relation[24]{};
    pickFromPool(FRIEND_RELATION_LABELS, static_cast<int32_t>(sizeof(FRIEND_RELATION_LABELS) / sizeof(FRIEND_RELATION_LABELS[0])), draft.characterRollSeed, 0x46524932, relation, sizeof(relation));
    const int32_t opinion = rollBaselineOpinion(draft.characterRollSeed, 0x46524933, 8, 38);
    seedGeneratedAgent(
        store.states[FRIEND_AGENT_SLOT_INDEX],
        displayName,
        relation,
        rollMotive(draft.characterRollSeed, 0x46524934),
        rollTrait(draft.characterRollSeed, 0x46524935),
        rollEmotion(draft.characterRollSeed, 0x46524936),
        opinion);
}

} // namespace

void rollCharacterSocialNetwork(CharacterDraft& draft) {
    const uint64_t seed = draft.characterRollSeed;
    draft.hasFamilyInCountry = rollPercentChance(seed, 0x46414D00, computeFamilyChancePercent(draft)) != 0;
    draft.hasFriendsInCountry = rollPercentChance(seed, 0x46524900, computeFriendChancePercent(draft)) != 0;
}

void buildStartingContactPreviewStore(const CharacterDraft& draft, CharacterAgentStore& store) {
    initializeCharacterAgentStore(store);
    spawnPersonalContactsFromDraft(draft, store);
}

void spawnPersonalContactsFromDraft(const CharacterDraft& draft, CharacterAgentStore& store) {
    store.states[FAMILY_AGENT_SLOT_INDEX] = CharacterAgentState{};
    store.states[FRIEND_AGENT_SLOT_INDEX] = CharacterAgentState{};
    if (draft.hasFamilyInCountry) {
        spawnFamilyContact(draft, store);
    }
    if (draft.hasFriendsInCountry) {
        spawnFriendContact(draft, store);
    }
}

void formatCharacterSocialSummary(const CharacterDraft& draft, char* outBuffer, size_t bufferSize) {
    if (outBuffer == nullptr || bufferSize == 0) {
        return;
    }
    if (!draft.hasFamilyInCountry && !draft.hasFriendsInCountry) {
        std::snprintf(outBuffer, bufferSize, "No family or friends in-country (you are on your own).");
        return;
    }
    if (draft.hasFamilyInCountry && draft.hasFriendsInCountry) {
        std::snprintf(outBuffer, bufferSize, "Family and friends in-country (contacts generated at start).");
        return;
    }
    if (draft.hasFamilyInCountry) {
        std::snprintf(outBuffer, bufferSize, "Family in-country only (friend contact not rolled).");
        return;
    }
    std::snprintf(outBuffer, bufferSize, "Friend in-country only (no family here).");
}

bool hasPersonalLodgingOption(const CharacterDraft& draft) {
    return draft.hasFamilyInCountry || draft.hasFriendsInCountry;
}

} // namespace Core
