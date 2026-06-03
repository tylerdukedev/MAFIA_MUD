#include "character/character_family.h"
#include "character/character_name_pools.h"
#include "character/player_profile.h"
#include "utils/seed_hash.h"
#include <algorithm>
#include <cstdio>
#include <cstring>

namespace Core {

namespace {

constexpr int32_t PRESENCE_SAME_BOROUGH_PERCENT = 55;
constexpr int32_t PRESENCE_OTHER_BOROUGH_PERCENT = 40;

FamilyMemberPresence rollFamilyMemberPresence(uint64_t seed, int32_t salt, bool hasFamilyInCountry) {
    if (!hasFamilyInCountry) {
        return FamilyMemberPresence::Abroad;
    }
    const uint32_t roll = Utils::hashSeedMix(seed, salt, 0x50524553) % 100U;
    if (roll < static_cast<uint32_t>(PRESENCE_SAME_BOROUGH_PERCENT)) {
        return FamilyMemberPresence::SameBorough;
    }
    if (roll < static_cast<uint32_t>(PRESENCE_SAME_BOROUGH_PERCENT + PRESENCE_OTHER_BOROUGH_PERCENT)) {
        return FamilyMemberPresence::OtherBoroughInCity;
    }
    return FamilyMemberPresence::Abroad;
}

void fillFamilyMember(
    FamilyMemberRecord& member,
    FamilyMemberRole role,
    HeritageId heritageId,
    NationalityId nationalityId,
    uint64_t seed,
    int32_t nameSalt,
    int32_t presenceSalt,
    bool hasFamilyInCountry,
    const FamilyCulturalProfile& culturalProfile) {
    member.role = role;
    std::strncpy(member.roleLabel, getFamilyMemberRoleLabel(role), sizeof(member.roleLabel) - 1);
    buildRandomCharacterName(member.displayName, sizeof(member.displayName), heritageId, nationalityId, seed, nameSalt);
    member.presence = rollFamilyMemberPresence(seed, presenceSalt, hasFamilyInCountry);
    member.isInCountry = member.presence != FamilyMemberPresence::Abroad;
    member.isAlive = true;
    if (role == FamilyMemberRole::Father) {
        member.baselineOpinion = 22 + static_cast<int32_t>(culturalProfile.filialDutyWeight * 18.0f);
        return;
    }
    if (role == FamilyMemberRole::Mother) {
        member.baselineOpinion = 28 + static_cast<int32_t>(culturalProfile.emotionalExpressiveness * 12.0f);
        return;
    }
    member.baselineOpinion = 10 + static_cast<int32_t>(culturalProfile.kinLoyaltyPressure * 8.0f);
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

void applyFamilyCulturalProfileToPlayer(const FamilyCulturalProfile& profile, PlayerProfile& playerProfile) {
    playerProfile.loyaltyBias.kinAlliancePreference = std::min(
        1.0f, playerProfile.loyaltyBias.kinAlliancePreference + profile.kinLoyaltyPressure * 0.14f);
    playerProfile.loyaltyBias.ethnicFactionResistance = std::min(
        1.0f, playerProfile.loyaltyBias.ethnicFactionResistance + profile.filialDutyWeight * 0.08f);
    playerProfile.culturalCompetency.inGroupNegotiation = std::min(
        1.0f, playerProfile.culturalCompetency.inGroupNegotiation + profile.emotionalExpressiveness * 0.07f);
    playerProfile.legitimacy.mainstreamSuspicion = std::min(
        1.0f, playerProfile.legitimacy.mainstreamSuspicion + profile.shameSensitivity * 0.06f);
    playerProfile.legitimacy.publicFacingJobAccess = std::min(
        1.0f, playerProfile.legitimacy.publicFacingJobAccess + profile.elderAuthorityWeight * 0.05f);
    playerProfile.networkAccess.ethnicNetwork = std::min(
        1.0f, playerProfile.networkAccess.ethnicNetwork + profile.kinLoyaltyPressure * 0.06f);
}

void formatFamilyCulturalGameplayLines(const FamilyCulturalProfile& profile, char* outBuffer, size_t bufferSize) {
    if (outBuffer == nullptr || bufferSize == 0) {
        return;
    }
    std::snprintf(
        outBuffer,
        bufferSize,
        "Duty %.0f%%: kin expect help; ignoring them erodes opinions faster.\n"
        "Express %.0f%%: family talks hit harder; reconciliations can swing more.\n"
        "Elder auth %.0f%%: parents/elers nudge job access and ethnic introductions.",
        profile.filialDutyWeight * 100.0f,
        profile.emotionalExpressiveness * 100.0f,
        profile.elderAuthorityWeight * 100.0f);
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

const char* getFamilyMemberPresenceLabel(FamilyMemberPresence presence) {
    switch (presence) {
    case FamilyMemberPresence::SameBorough:
        return "your borough";
    case FamilyMemberPresence::OtherBoroughInCity:
        return "elsewhere in the city";
    default:
        return "abroad";
    }
}

void generatePlayerFamilyTree(CharacterDraft& draft) {
    draft.familyMemberCount = 0;
    for (int32_t index = 0; index < MAX_FAMILY_MEMBER_COUNT; ++index) {
        draft.familyMembers[index] = FamilyMemberRecord{};
    }
    buildFamilyCulturalProfile(draft.heritageId, draft.nationalityId, draft.familyCulturalProfile);
    const uint64_t seed = draft.characterRollSeed;
    int32_t memberIndex = 0;
    fillFamilyMember(
        draft.familyMembers[memberIndex++],
        FamilyMemberRole::Father,
        draft.heritageId,
        draft.nationalityId,
        seed,
        0x46415431,
        0x46415441,
        draft.hasFamilyInCountry,
        draft.familyCulturalProfile);
    fillFamilyMember(
        draft.familyMembers[memberIndex++],
        FamilyMemberRole::Mother,
        draft.heritageId,
        draft.nationalityId,
        seed,
        0x4D4F5431,
        0x4D4F5441,
        draft.hasFamilyInCountry,
        draft.familyCulturalProfile);
    if (draft.generationId != GenerationId::Immigrant) {
        fillFamilyMember(
            draft.familyMembers[memberIndex++],
            FamilyMemberRole::Sibling,
            draft.heritageId,
            draft.nationalityId,
            seed,
            0x53494231,
            0x53494241,
            draft.hasFamilyInCountry,
            draft.familyCulturalProfile);
    }
    draft.familyMemberCount = memberIndex;
}

} // namespace Core
