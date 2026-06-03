#pragma once

#include "character/character_types.h"
#include <cstdint>

namespace Core {

struct CharacterDraft;

constexpr int32_t MAX_FAMILY_MEMBER_COUNT = 6;

enum class FamilyMemberRole : uint8_t {
    Father = 0,
    Mother = 1,
    Sibling = 2,
    Spouse = 3,
    Child = 4,
    Extended = 5,
};

struct FamilyCulturalProfile {
    float filialDutyWeight = 0.5f;
    float emotionalExpressiveness = 0.5f;
    float elderAuthorityWeight = 0.5f;
    float shameSensitivity = 0.5f;
    float kinLoyaltyPressure = 0.5f;
};

struct FamilyMemberRecord {
    FamilyMemberRole role = FamilyMemberRole::Extended;
    char roleLabel[20]{};
    char displayName[32]{};
    bool isInCountry = false;
    bool isAlive = true;
    int32_t baselineOpinion = 20;
    int32_t baselineTrust = 40;
    int32_t baselineRespect = 35;
};

void buildFamilyCulturalProfile(HeritageId heritageId, NationalityId nationalityId, FamilyCulturalProfile& outProfile);
void generatePlayerFamilyTree(CharacterDraft& draft);
const char* getFamilyMemberRoleLabel(FamilyMemberRole role);

} // namespace Core
