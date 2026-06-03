#pragma once

#include "character/character_draft.h"
#include <cstddef>

namespace Core {

constexpr int32_t CHARACTER_NAME_PART_MAX_LENGTH = 20;
constexpr int32_t CHARACTER_LAST_NAME_MAX_LENGTH = 24;

void sanitizeCharacterNamePart(char* buffer, size_t bufferSize);
void rebuildCharacterFullName(CharacterDraft& draft);
bool isCharacterNameValid(const CharacterDraft& draft);
void normalizeCharacterDraftNames(CharacterDraft& draft);

} // namespace Core
