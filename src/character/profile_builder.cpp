#include "character/profile_builder.h"
#include "character/character_tables.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace Core {

namespace {
constexpr float MIN_TRAIT = 0.0f;
constexpr float MAX_TRAIT = 1.0f;

float clampTrait(float value) {
    return std::max(MIN_TRAIT, std::min(MAX_TRAIT, value));
}

void applyGenerationBase(GenerationId generationId, PlayerProfile& profile) {
    switch (generationId) {
    case GenerationId::Immigrant:
        profile.networkAccess = {0.85f, 0.25f, 0.15f, 0.30f, 0.90f};
        profile.legitimacy = {0.20f, 0.15f, 0.15f, 0.75f};
        profile.loyaltyBias = {0.85f, 0.80f, 0.70f, 0.20f};
        profile.culturalCompetency = {0.85f, 0.35f, 0.70f, 0.40f, 0.20f};
        profile.opportunityPaths = {0.90f, 0.45f, 0.20f, 0.10f};
        break;
    case GenerationId::FirstGeneration:
        profile.networkAccess = {0.60f, 0.45f, 0.35f, 0.45f, 0.50f};
        profile.legitimacy = {0.50f, 0.45f, 0.45f, 0.45f};
        profile.loyaltyBias = {0.60f, 0.60f, 0.45f, 0.45f};
        profile.culturalCompetency = {0.65f, 0.60f, 0.35f, 0.65f, 0.70f};
        profile.opportunityPaths = {0.55f, 0.75f, 0.45f, 0.30f};
        break;
    case GenerationId::SecondGeneration:
        profile.networkAccess = {0.35f, 0.65f, 0.60f, 0.65f, 0.25f};
        profile.legitimacy = {0.70f, 0.70f, 0.75f, 0.35f};
        profile.loyaltyBias = {0.35f, 0.40f, 0.30f, 0.70f};
        profile.culturalCompetency = {0.50f, 0.70f, 0.25f, 0.55f, 0.45f};
        profile.opportunityPaths = {0.35f, 0.55f, 0.80f, 0.55f};
        break;
    case GenerationId::ThirdGenerationPlus:
        profile.networkAccess = {0.15f, 0.75f, 0.70f, 0.80f, 0.10f};
        profile.legitimacy = {0.90f, 0.90f, 0.90f, 0.15f};
        profile.loyaltyBias = {0.15f, 0.20f, 0.15f, 0.85f};
        profile.culturalCompetency = {0.30f, 0.80f, 0.10f, 0.40f, 0.20f};
        profile.opportunityPaths = {0.15f, 0.35f, 0.65f, 0.90f};
        break;
    default:
        break;
    }
}

void applyHeritageNudges(HeritageId heritageId, PlayerProfile& profile) {
    switch (heritageId) {
    case HeritageId::Italian:
    case HeritageId::Sicilian:
        profile.networkAccess.ethnicNetwork = clampTrait(profile.networkAccess.ethnicNetwork + 0.10f);
        profile.opportunityPaths.streetCrimePath = clampTrait(profile.opportunityPaths.streetCrimePath + 0.05f);
        break;
    case HeritageId::Irish:
        profile.networkAccess.lawEnforcementChannel = clampTrait(profile.networkAccess.lawEnforcementChannel + 0.08f);
        profile.networkAccess.politicalMachine = clampTrait(profile.networkAccess.politicalMachine + 0.05f);
        break;
    case HeritageId::Jewish:
        profile.networkAccess.politicalMachine = clampTrait(profile.networkAccess.politicalMachine + 0.08f);
        profile.networkAccess.businessAssociation = clampTrait(profile.networkAccess.businessAssociation + 0.05f);
        break;
    case HeritageId::Chinese:
        profile.networkAccess.importPipeline = clampTrait(profile.networkAccess.importPipeline + 0.08f);
        profile.culturalCompetency.inGroupNegotiation = clampTrait(profile.culturalCompetency.inGroupNegotiation + 0.05f);
        break;
    case HeritageId::Polish:
    case HeritageId::Russian:
        profile.loyaltyBias.kinAlliancePreference = clampTrait(profile.loyaltyBias.kinAlliancePreference + 0.05f);
        break;
    case HeritageId::Greek:
        profile.networkAccess.businessAssociation = clampTrait(profile.networkAccess.businessAssociation + 0.06f);
        break;
    case HeritageId::AfricanAmerican:
        profile.legitimacy.mainstreamSuspicion = clampTrait(profile.legitimacy.mainstreamSuspicion + 0.08f);
        profile.culturalCompetency.outGroupNegotiation = clampTrait(profile.culturalCompetency.outGroupNegotiation + 0.05f);
        break;
    case HeritageId::Mexican:
        profile.networkAccess.ethnicNetwork = clampTrait(profile.networkAccess.ethnicNetwork + 0.08f);
        profile.opportunityPaths.organizerPath = clampTrait(profile.opportunityPaths.organizerPath + 0.04f);
        break;
    default:
        break;
    }
}

void applyNationalityNudges(NationalityId nationalityId, PlayerProfile& profile) {
    if (nationalityId == NationalityId::American) {
        profile.legitimacy.publicFacingJobAccess = clampTrait(profile.legitimacy.publicFacingJobAccess + 0.05f);
        profile.culturalCompetency.languageAccess = clampTrait(profile.culturalCompetency.languageAccess + 0.10f);
        return;
    }
    profile.networkAccess.importPipeline = clampTrait(profile.networkAccess.importPipeline + 0.05f);
    profile.culturalCompetency.crossEthnicPenalty = clampTrait(profile.culturalCompetency.crossEthnicPenalty + 0.03f);
}

void applyAgeNudges(int32_t age, PlayerProfile& profile) {
    const int32_t clampedAge = std::max(CHARACTER_CREATION_MIN_AGE, std::min(CHARACTER_CREATION_MAX_AGE, age));
    const float youthFactor = static_cast<float>(CHARACTER_CREATION_MAX_AGE - clampedAge) / static_cast<float>(CHARACTER_CREATION_MAX_AGE - CHARACTER_CREATION_MIN_AGE);
    profile.opportunityPaths.streetCrimePath = clampTrait(profile.opportunityPaths.streetCrimePath + youthFactor * 0.05f);
    profile.opportunityPaths.institutionalPath = clampTrait(profile.opportunityPaths.institutionalPath - youthFactor * 0.05f);
    profile.opportunityPaths.corporatePath = clampTrait(profile.opportunityPaths.corporatePath - youthFactor * 0.05f);
}

void applyBackgroundNudges(BackgroundId backgroundId, PlayerProfile& profile) {
    switch (backgroundId) {
    case BackgroundId::StreetHustler:
        profile.opportunityPaths.streetCrimePath = clampTrait(profile.opportunityPaths.streetCrimePath + 0.08f);
        profile.legitimacy.mainstreamSuspicion = clampTrait(profile.legitimacy.mainstreamSuspicion + 0.05f);
        break;
    case BackgroundId::NeighborhoodOrganizer:
        profile.opportunityPaths.organizerPath = clampTrait(profile.opportunityPaths.organizerPath + 0.08f);
        profile.networkAccess.politicalMachine = clampTrait(profile.networkAccess.politicalMachine + 0.05f);
        break;
    case BackgroundId::Bookkeeper:
        profile.opportunityPaths.corporatePath = clampTrait(profile.opportunityPaths.corporatePath + 0.08f);
        profile.legitimacy.shellCompanyEase = clampTrait(profile.legitimacy.shellCompanyEase + 0.05f);
        break;
    default:
        break;
    }
}
} // namespace

void initializeCharacterDraftDefaults(CharacterDraft& draft) {
    if (draft.hasInitializedDefaults) {
        return;
    }
    std::snprintf(draft.nameBuffer, sizeof(draft.nameBuffer), "%s", "New Boss");
    draft.nationalityId = NationalityId::American;
    draft.heritageId = HeritageId::Italian;
    draft.generationId = GenerationId::FirstGeneration;
    draft.age = 21;
    draft.backgroundId = BackgroundId::StreetHustler;
    draft.selectedBoroughIndex = 0;
    draft.hasInitializedDefaults = true;
}

PlayerProfile buildPlayerProfile(const CharacterDraft& draft) {
    PlayerProfile profile{};
    profile.draft = draft;
    applyGenerationBase(draft.generationId, profile);
    applyHeritageNudges(draft.heritageId, profile);
    applyNationalityNudges(draft.nationalityId, profile);
    applyAgeNudges(draft.age, profile);
    applyBackgroundNudges(draft.backgroundId, profile);
    return profile;
}

void buildIdentityLabel(char* outBuffer, size_t bufferSize, const CharacterDraft& draft) {
    if (outBuffer == nullptr || bufferSize == 0) {
        return;
    }
    outBuffer[0] = '\0';
    const std::string_view generationName = getGenerationName(draft.generationId);
    const std::string_view heritageName = getHeritageName(draft.heritageId);
    const std::string_view nationalityName = getNationalityName(draft.nationalityId);
    if (draft.generationId == GenerationId::Immigrant) {
        std::snprintf(outBuffer, bufferSize, "%.*s Immigrant", static_cast<int>(heritageName.size()), heritageName.data());
        return;
    }
    if (draft.nationalityId == NationalityId::American) {
        std::snprintf(
            outBuffer,
            bufferSize,
            "%.*s %.*s-American",
            static_cast<int>(generationName.size()),
            generationName.data(),
            static_cast<int>(heritageName.size()),
            heritageName.data());
        return;
    }
    std::snprintf(
        outBuffer,
        bufferSize,
        "%.*s %.*s (%.*s)",
        static_cast<int>(generationName.size()),
        generationName.data(),
        static_cast<int>(heritageName.size()),
        heritageName.data(),
        static_cast<int>(nationalityName.size()),
        nationalityName.data());
}

void buildCharacterDescription(char* outBuffer, size_t bufferSize, const CharacterDraft& draft) {
    if (outBuffer == nullptr || bufferSize == 0) {
        return;
    }
    char identityBuffer[96];
    buildIdentityLabel(identityBuffer, sizeof(identityBuffer), draft);
    const std::string_view boroughName = getBoroughPreferenceName(draft.selectedBoroughIndex);
    const std::string_view backgroundName = getBackgroundName(draft.backgroundId);
    std::snprintf(
        outBuffer,
        bufferSize,
        "%s, age %d - %s from %.*s. Background: %.*s.",
        draft.nameBuffer,
        draft.age,
        identityBuffer,
        static_cast<int>(boroughName.size()),
        boroughName.data(),
        static_cast<int>(backgroundName.size()),
        backgroundName.data());
}

} // namespace Core
