#pragma once

#include "character/character_draft.h"
#include "character/player_profile.h"
#include <cstddef>

namespace Core {

void initializeCharacterDraftDefaults(CharacterDraft& draft);
PlayerProfile buildPlayerProfile(const CharacterDraft& draft);
void buildCharacterDescription(char* outBuffer, size_t bufferSize, const CharacterDraft& draft);
void buildIdentityLabel(char* outBuffer, size_t bufferSize, const CharacterDraft& draft);

} // namespace Core
