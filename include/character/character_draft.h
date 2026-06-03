#pragma once

#include "character/character_types.h"
#include <cstdint>

namespace Core {

struct CharacterDraft {
    bool hasInitializedDefaults = false;
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
};

} // namespace Core
