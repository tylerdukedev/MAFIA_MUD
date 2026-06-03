#pragma once

#include "character/character_draft.h"
#include <cstdint>

namespace Core {

void rollCharacterStartPlacement(CharacterDraft& draft, uint64_t worldSeed);
void applyCharacterSocialFlags(CharacterDraft& draft);

} // namespace Core
