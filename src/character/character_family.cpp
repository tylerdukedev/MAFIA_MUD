#include "character/character_family.h"
#include "character/character_social_network.h"
#include "utils/seed_hash.h"
#include <algorithm>
#include <cstring>

namespace Core {

namespace {

constexpr const char* GIVEN_ITALIAN[] = {"Vito", "Rosa", "Antonio", "Giulia", "Salvatore"};
constexpr const char* GIVEN_IRISH[] = {"Patrick", "Maeve", "Sean", "Bridget", "Tommy"};
constexpr const char* GIVEN_JEWISH[] = {"Morris", "Ruth", "Hyman", "Sadie", "Louis"};
constexpr const char* GIVEN_GENERIC[] = {"James", "Mary", "John", "Anna", "Michael"};
constexpr const char* SURNAME_ITALIAN[] = {"Marino", "Lombardi", "Ricci"};
constexpr const char* SURNAME_IRISH[] = {"Sullivan", "Murphy", "Kelly"};
constexpr const char* SURNAME_GENERIC[] = {"Johnson", "Williams", "Brown"};

void pickName(char* outBuffer, size_t bufferSize, const char* const* pool, int32_t poolCount, uint64_t seed, int32_t salt) {
    if (poolCount <= 0 || outBuffer == nullptr || bufferSize == 0) {
        return;
    }
    const uint32_t index = Utils::hashSeedMix(seed, salt, 0x4E414D45) % static_cast<uint32_t>(poolCount);
    std::strncpy(outBuffer, pool[index], bufferSize - 1);
    outBuffer[bufferSize - 1] = '\0';
}

void buildMemberName(
    char* outBuffer,
    size_t bufferSize,
    HeritageId heritageId,
    uint64_t seed,
    int32_t salt,
    const char* surname) {
    const char* const* givenPool = GIVEN_GENERIC;
    int32_t givenCount = static_cast<int32_t>(sizeof(GIVEN_GENERIC) / sizeof(GIVEN_GENERIC[0]));
    if (heritageId == HeritageId::Italian || heritageId == HeritageId::Sicilian) {
        givenPool = GIVEN_ITALIAN;
        givenCount = static_cast<int32_t>(sizeof(GIVEN_ITALIAN) / sizeof(GIVEN_ITALIAN[0]));
    } else if (heritageId == HeritageId::Irish) {
        givenPool = GIVEN_IRISH;
        givenCount = static_cast<int32_t>(sizeof(GIVEN_IRISH) / sizeof(GIVEN_IRISH[0]));
    } else if (heritageId == HeritageId::Jewish) {
        givenPool = GIVEN_JEWISH;
        givenCount = static_cast<int32_t>(sizeof(GIVEN_JEWISH) / sizeof(GIVEN_JEWISH[0]));
    }
    char given[20]{};
    pickName(given, sizeof(given), givenPool, givenCount, seed, salt);
    std::snprintf(outBuffer, bufferSize, "%s %s", given, surname);
}

} // namespace

void buildFamilyCulturalProfile(HeritageId heritageId, NationalityId nationalityId, FamilyCulturalProfile& outProfile) {
    outProfile = FamilyCulturalProfile{};
    if (heritageId == HeritageId::Italian || heritageId == HeritageId::Sicilian) {
        outProfile.filialDutyWeight = 0.82f;
        outProfile.emotionalExpressiveness = 0.72f;
        outProfile.elderAuthorityWeight = 0.78f;
        outProfile.shameSensitivity = 0.70f;
        outProfile.kinLoyaltyPressure = 0.85f;
        return;
    }
    if (heritageId == HeritageId::Irish) {
        outProfile.filialDutyWeight = 0.68f;
        outProfile.emotionalExpressiveness = 0.55f;
        outProfile.elderAuthorityWeight = 0.62f;
        outProfile.shameSensitivity = 0.58f;
        outProfile.kinLoyaltyPressure = 0.74f;
        return;
    }
    if (heritageId == HeritageId::Jewish) {
        outProfile.filialDutyWeight = 0.75f;
        outProfile.emotionalExpressiveness = 0.48f;
        outProfile.elderAuthorityWeight = 0.80f;
        outProfile.shameSensitivity = 0.76f;
        outProfile.kinLoyaltyPressure = 0.80f;
        return;
    }
    if (heritageId == HeritageId::Chinese) {
        outProfile.filialDutyWeight = 0.88f;
        outProfile.emotionalExpressiveness = 0.40f;
        outProfile.elderAuthorityWeight = 0.90f;
        outProfile.shameSensitivity = 0.82f;
        outProfile.kinLoyaltyPressure = 0.86f;
        return;
    }
    if (nationalityId == NationalityId::American && heritageId == HeritageId::AfricanAmerican) {
        outProfile.filialDutyWeight = 0.70f;
        outProfile.emotionalExpressiveness = 0.65f;
        outProfile.elderAuthorityWeight = 0.72f;
        outProfile.shameSensitivity = 0.60f;
        outProfile.kinLoyaltyPressure = 0.78f;
        return;
    }
    outProfile.filialDutyWeight = 0.55f;
    outProfile.emotionalExpressiveness = 0.50f;
    outProfile.elderAuthorityWeight = 0.50f;
    outProfile.shameSensitivity = 0.45f;
    outProfile.kinLoyaltyPressure = 0.55f;
}

const char* getFamilyMemberRoleLabel(FamilyMemberRole role) {
    switch (role) {
    case FamilyMemberRole::Father:
        return "Father";
    case FamilyMemberRole::Mother:
        return "Mother";
    case FamilyMemberRole::Sibling:
        return "Sibling";
    case FamilyMemberRole::Spouse:
        return "Spouse";
    case FamilyMemberRole::Child:
        return "Child";
    default:
        return "Kin";
    }
}

void generatePlayerFamilyTree(CharacterDraft& draft) {
    draft.familyMemberCount = 0;
    for (int32_t index = 0; index < MAX_FAMILY_MEMBER_COUNT; ++index) {
        draft.familyMembers[index] = FamilyMemberRecord{};
    }
    buildFamilyCulturalProfile(draft.heritageId, draft.nationalityId, draft.familyCulturalProfile);
    char surname[24]{};
    if (draft.lastName[0] != '\0') {
        std::strncpy(surname, draft.lastName, sizeof(surname) - 1);
    } else {
        pickName(surname, sizeof(surname), SURNAME_GENERIC, static_cast<int32_t>(sizeof(SURNAME_GENERIC) / sizeof(SURNAME_GENERIC[0])), draft.characterRollSeed, 0x534E5231);
    }
    const uint64_t seed = draft.characterRollSeed;
    int32_t memberIndex = 0;
    FamilyMemberRecord father{};
    father.role = FamilyMemberRole::Father;
    std::strncpy(father.roleLabel, getFamilyMemberRoleLabel(father.role), sizeof(father.roleLabel) - 1);
    buildMemberName(father.displayName, sizeof(father.displayName), draft.heritageId, seed, 0x46415431, surname);
    father.isInCountry = draft.hasFamilyInCountry;
    father.baselineOpinion = 22 + static_cast<int32_t>(draft.familyCulturalProfile.filialDutyWeight * 18.0f);
    draft.familyMembers[memberIndex++] = father;
    FamilyMemberRecord mother{};
    mother.role = FamilyMemberRole::Mother;
    std::strncpy(mother.roleLabel, getFamilyMemberRoleLabel(mother.role), sizeof(mother.roleLabel) - 1);
    buildMemberName(mother.displayName, sizeof(mother.displayName), draft.heritageId, seed, 0x4D4F5431, surname);
    mother.isInCountry = draft.hasFamilyInCountry;
    mother.baselineOpinion = 28 + static_cast<int32_t>(draft.familyCulturalProfile.emotionalExpressiveness * 12.0f);
    draft.familyMembers[memberIndex++] = mother;
    if (draft.generationId != GenerationId::Immigrant) {
        FamilyMemberRecord sibling{};
        sibling.role = FamilyMemberRole::Sibling;
        std::strncpy(sibling.roleLabel, getFamilyMemberRoleLabel(sibling.role), sizeof(sibling.roleLabel) - 1);
        buildMemberName(sibling.displayName, sizeof(sibling.displayName), draft.heritageId, seed, 0x53494231, surname);
        sibling.isInCountry = draft.hasFamilyInCountry;
        sibling.baselineOpinion = 10;
        draft.familyMembers[memberIndex++] = sibling;
    }
    draft.familyMemberCount = memberIndex;
}

} // namespace Core
