#include "character/character_start.h"
#include "character/character_types.h"
#include "game/player_wallet.h"
#include "world/landmark_table.h"
#include "utils/seed_hash.h"

namespace Core {

void applyCharacterSocialFlags(CharacterDraft& draft) {
    draft.hasFamilyInCountry = draft.generationId != GenerationId::Immigrant;
    draft.hasFriendsInCountry = draft.backgroundId == BackgroundId::StreetHustler
        || draft.backgroundId == BackgroundId::NeighborhoodOrganizer
        || draft.generationId == GenerationId::SecondGeneration;
}

void rollCharacterStartPlacement(CharacterDraft& draft, uint64_t worldSeed) {
    draft.characterRollSeed = worldSeed ^ Utils::hashSeedMix(worldSeed, draft.selectedBoroughIndex, 0x53544152);
    draft.startingCashCents = rollStartingCashCents(draft.characterRollSeed, draft.selectedBoroughIndex);
    const RegionId regionId = regionIdFromBoroughPreferenceIndex(draft.selectedBoroughIndex);
    draft.startingCityLandmarkIndex = pickRandomLandmarkIndexInRegion(regionId, draft.characterRollSeed, 0x43495459);
    applyCharacterSocialFlags(draft);
}

} // namespace Core
