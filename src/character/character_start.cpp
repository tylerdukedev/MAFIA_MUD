#include "character/character_start.h"
#include "character/character_family.h"
#include "character/character_name.h"
#include "character/character_social_network.h"
#include "game/player_wallet.h"
#include "world/landmark_table.h"
#include "utils/seed_hash.h"

namespace Core {

void rollCharacterStartPlacement(CharacterDraft& draft, uint64_t worldSeed) {
    draft.characterRollSeed = worldSeed ^ Utils::hashSeedMix(worldSeed, draft.selectedBoroughIndex, 0x53544152);
    draft.startingCashCents = rollStartingCashCents(draft.characterRollSeed, draft.selectedBoroughIndex);
    rollCharacterSocialNetwork(draft);
    normalizeCharacterDraftNames(draft);
    generatePlayerFamilyTree(draft);
    const RegionId regionId = regionIdFromBoroughPreferenceIndex(draft.selectedBoroughIndex);
    draft.startingCityLandmarkIndex = pickRandomLandmarkIndexInRegion(regionId, draft.characterRollSeed, 0x43495459);
}

} // namespace Core
