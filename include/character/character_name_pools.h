#pragma once

#include "character/character_types.h"
#include <cstddef>
#include <cstdint>

namespace Core {

struct CharacterNamePools {
    const char* const* givenNames = nullptr;
    int32_t givenCount = 0;
    const char* const* surnames = nullptr;
    int32_t surnameCount = 0;
};

CharacterNamePools pickCharacterNamePools(HeritageId heritageId, NationalityId nationalityId);
void pickCharacterNameFromPool(
    char* outBuffer,
    size_t bufferSize,
    const char* const* pool,
    int32_t poolCount,
    uint64_t seed,
    int32_t salt);
void buildRandomCharacterName(
    char* outBuffer,
    size_t bufferSize,
    HeritageId heritageId,
    NationalityId nationalityId,
    uint64_t seed,
    int32_t salt);

} // namespace Core
