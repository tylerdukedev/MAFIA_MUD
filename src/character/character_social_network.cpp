#include "character/character_social_network.h"
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

constexpr const char* GIVEN_ITALIAN[] = {"Vito", "Rosa", "Antonio", "Giulia", "Salvatore", "Nina"};
constexpr const char* GIVEN_IRISH[] = {"Patrick", "Maeve", "Sean", "Bridget", "Tommy", "Kathleen"};
constexpr const char* GIVEN_JEWISH[] = {"Morris", "Ruth", "Hyman", "Sadie", "Louis", "Esther"};
constexpr const char* GIVEN_CHINESE[] = {"Wing", "Mei", "Lok", "Sui", "Chen", "Lily"};
constexpr const char* GIVEN_POLISH[] = {"Stanislaw", "Helena", "Jozef", "Wanda", "Kazimierz", "Anya"};
constexpr const char* GIVEN_GENERIC[] = {"James", "Mary", "John", "Anna", "Michael", "Sarah", "Frank", "Lena"};

constexpr const char* SURNAME_ITALIAN[] = {"Marino", "Lombardi", "Ricci", "Esposito", "Moretti"};
constexpr const char* SURNAME_IRISH[] = {"Sullivan", "Murphy", "Kelly", "Doyle", "Flynn"};
constexpr const char* SURNAME_JEWISH[] = {"Katz", "Rosen", "Levy", "Goldman", "Weiss"};
constexpr const char* SURNAME_CHINESE[] = {"Wong", "Lee", "Chan", "Lum", "Fong"};
constexpr const char* SURNAME_POLISH[] = {"Kowalski", "Nowak", "Wisniewski", "Zielinski"};
constexpr const char* SURNAME_GENERIC[] = {"Johnson", "Williams", "Brown", "Davis", "Miller", "Wilson"};

constexpr const char* FAMILY_RELATION_LABELS[] = {"Uncle", "Aunt", "Cousin", "Older brother", "Older sister", "In-law"};
constexpr const char* FRIEND_RELATION_LABELS[] = {"Childhood friend", "Tenement neighbor", "Work buddy", "Boarding roommate"};

struct NamePools {
    const char* const* givenNames;
    int32_t givenCount;
    const char* const* surnames;
    int32_t surnameCount;
};

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

NamePools pickNamePools(HeritageId heritageId) {
    if (heritageId == HeritageId::Italian || heritageId == HeritageId::Sicilian) {
        return {GIVEN_ITALIAN, static_cast<int32_t>(sizeof(GIVEN_ITALIAN) / sizeof(GIVEN_ITALIAN[0])), SURNAME_ITALIAN, static_cast<int32_t>(sizeof(SURNAME_ITALIAN) / sizeof(SURNAME_ITALIAN[0]))};
    }
    if (heritageId == HeritageId::Irish) {
        return {GIVEN_IRISH, static_cast<int32_t>(sizeof(GIVEN_IRISH) / sizeof(GIVEN_IRISH[0])), SURNAME_IRISH, static_cast<int32_t>(sizeof(SURNAME_IRISH) / sizeof(SURNAME_IRISH[0]))};
    }
    if (heritageId == HeritageId::Jewish) {
        return {GIVEN_JEWISH, static_cast<int32_t>(sizeof(GIVEN_JEWISH) / sizeof(GIVEN_JEWISH[0])), SURNAME_JEWISH, static_cast<int32_t>(sizeof(SURNAME_JEWISH) / sizeof(SURNAME_JEWISH[0]))};
    }
    if (heritageId == HeritageId::Chinese) {
        return {GIVEN_CHINESE, static_cast<int32_t>(sizeof(GIVEN_CHINESE) / sizeof(GIVEN_CHINESE[0])), SURNAME_CHINESE, static_cast<int32_t>(sizeof(SURNAME_CHINESE) / sizeof(SURNAME_CHINESE[0]))};
    }
    if (heritageId == HeritageId::Polish || heritageId == HeritageId::Russian || heritageId == HeritageId::Greek) {
        return {GIVEN_POLISH, static_cast<int32_t>(sizeof(GIVEN_POLISH) / sizeof(GIVEN_POLISH[0])), SURNAME_POLISH, static_cast<int32_t>(sizeof(SURNAME_POLISH) / sizeof(SURNAME_POLISH[0]))};
    }
    return {GIVEN_GENERIC, static_cast<int32_t>(sizeof(GIVEN_GENERIC) / sizeof(GIVEN_GENERIC[0])), SURNAME_GENERIC, static_cast<int32_t>(sizeof(SURNAME_GENERIC) / sizeof(SURNAME_GENERIC[0]))};
}

void pickFromPool(const char* const* pool, int32_t poolCount, uint64_t seed, int32_t salt, char* outBuffer, size_t bufferSize) {
    if (poolCount <= 0 || outBuffer == nullptr || bufferSize == 0) {
        return;
    }
    const uint32_t index = Utils::hashSeedMix(seed, salt, 0x4E414D45) % static_cast<uint32_t>(poolCount);
    std::strncpy(outBuffer, pool[static_cast<size_t>(index)], bufferSize - 1);
    outBuffer[bufferSize - 1] = '\0';
}

void buildGeneratedDisplayName(
    char* outBuffer,
    size_t bufferSize,
    const NamePools& pools,
    uint64_t seed,
    int32_t salt) {
    char given[24]{};
    char surname[24]{};
    pickFromPool(pools.givenNames, pools.givenCount, seed, salt, given, sizeof(given));
    pickFromPool(pools.surnames, pools.surnameCount, seed, salt + 17, surname, sizeof(surname));
    std::snprintf(outBuffer, bufferSize, "%s %s", given, surname);
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
    const NamePools pools = pickNamePools(draft.heritageId);
    char displayName[32]{};
    buildGeneratedDisplayName(displayName, sizeof(displayName), pools, draft.characterRollSeed, 0x46414D31);
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
    const NamePools pools = pickNamePools(draft.heritageId);
    char displayName[32]{};
    buildGeneratedDisplayName(displayName, sizeof(displayName), pools, draft.characterRollSeed, 0x46524931);
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
