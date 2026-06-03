#include "character/character_start.h"
#include "game/player_wallet.h"
#include "world/landmark_table.h"
#include "utils/seed_hash.h"

namespace Core {

void rollCharacterStartPlacement(CharacterDraft& draft, uint64_t worldSeed) {
    draft.characterRollSeed = worldSeed ^ Utils::hashSeedMix(worldSeed, draft.selectedBoroughIndex, 0x53544152);
    draft.startingCashCents = rollStartingCashCents(draft.characterRollSeed, draft.selectedBoroughIndex);
    const RegionId regionId = regionIdFromBoroughPreferenceIndex(draft.selectedBoroughIndex);
    draft.startingCityLandmarkIndex = pickRandomLandmarkIndexInRegion(regionId, draft.characterRollSeed, 0x43495459);
}

} // namespace Core
