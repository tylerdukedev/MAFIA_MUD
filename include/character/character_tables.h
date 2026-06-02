#pragma once

#include "character/character_types.h"
#include <string_view>

namespace Core {

std::string_view getNationalityName(NationalityId nationalityId);
std::string_view getHeritageName(HeritageId heritageId);
std::string_view getGenerationName(GenerationId generationId);
std::string_view getBackgroundName(BackgroundId backgroundId);
std::string_view getGenerationRoleSummary(GenerationId generationId);
std::string_view getBoroughPreferenceName(int32_t boroughIndex);
int32_t getNationalityCount();
int32_t getHeritageCount();
int32_t getGenerationCount();
int32_t getBackgroundCount();
int32_t getBoroughPreferenceCount();
const char* getNationalityLabel(int32_t index);
const char* getHeritageLabel(int32_t index);
const char* getGenerationLabel(int32_t index);
const char* getBackgroundLabel(int32_t index);
const char* getBoroughPreferenceLabel(int32_t index);
NationalityId nationalityIdFromIndex(int32_t index);
HeritageId heritageIdFromIndex(int32_t index);
GenerationId generationIdFromIndex(int32_t index);
BackgroundId backgroundIdFromIndex(int32_t index);
NationalityId parseNationalityName(std::string_view name);
HeritageId parseHeritageName(std::string_view name);
GenerationId parseGenerationName(std::string_view name);

} // namespace Core
