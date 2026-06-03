#include "character/character_start.h"
#include "character/character_family.h"
#include "character/character_name.h"
#include "character/character_social_network.h"
#include "game/player_wallet.h"
#include "world/landmark_table.h"
#include "utils/seed_hash.h"

namespace Core {

void rollCharacterCreationPreview(CharacterDraft& draft, uint64_t worldSeed, int32_t rollRevision) {
    const int32_t mixSalt = draft.selectedBoroughIndex
        + static_cast<int32_t>(draft.heritageId) * 17
        + static_cast<int32_t>(draft.nationalityId) * 31
        + static_cast<int32_t>(draft.generationId) * 53
        + static_cast<int32_t>(draft.backgroundId) * 71
        + rollRevision * 97;
    draft.characterRollSeed = Utils::hashSeedMix(worldSeed, mixSalt, 0x43524541);
    rollCharacterSocialNetwork(draft);
    generatePlayerFamilyTree(draft);
}

void rollCharacterStartPlacement(CharacterDraft& draft, uint64_t worldSeed) {
    draft.startingCashCents = rollStartingCashCents(draft.characterRollSeed, draft.selectedBoroughIndex);
    normalizeCharacterDraftNames(draft);
    const RegionId regionId = regionIdFromBoroughPreferenceIndex(draft.selectedBoroughIndex);
    draft.startingCityLandmarkIndex = pickRandomLandmarkIndexInRegion(regionId, draft.characterRollSeed, 0x43495459);
}

} // namespace Core
