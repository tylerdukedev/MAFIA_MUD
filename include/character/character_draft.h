#pragma once

#include "character/character_family.h"
#include "character/character_types.h"
#include <cstdint>

namespace Core {

struct CharacterDraft {
    bool hasInitializedDefaults = false;
    char firstName[20]{};
    char middleName[20]{};
    char lastName[24]{};
    char nameBuffer[32]{};
    NationalityId nationalityId = NationalityId::American;
    HeritageId heritageId = HeritageId::Italian;
    GenerationId generationId = GenerationId::FirstGeneration;
    int32_t age = 21;
    BackgroundId backgroundId = BackgroundId::StreetHustler;
    int32_t selectedBoroughIndex = 0;
    int32_t startingCityLandmarkIndex = -1;
    int64_t startingCashCents = 0;
    uint64_t characterRollSeed = 0;
    bool hasFamilyInCountry = false;
    bool hasFriendsInCountry = false;
    FamilyCulturalProfile familyCulturalProfile{};
    FamilyMemberRecord familyMembers[MAX_FAMILY_MEMBER_COUNT]{};
    int32_t familyMemberCount = 0;
    uint8_t mapMarkerColorR = 220;
    uint8_t mapMarkerColorG = 180;
    uint8_t mapMarkerColorB = 60;
};

} // namespace Core
