#pragma once

#include "character/character_draft.h"

namespace Core {

struct NetworkAccessProfile {
    float ethnicNetwork = 0.0f;
    float politicalMachine = 0.0f;
    float lawEnforcementChannel = 0.0f;
    float businessAssociation = 0.0f;
    float importPipeline = 0.0f;
};

struct LegitimacyProfile {
    float policeAttentionDecay = 0.0f;
    float shellCompanyEase = 0.0f;
    float publicFacingJobAccess = 0.0f;
    float mainstreamSuspicion = 0.0f;
};

struct LoyaltyBiasProfile {
    float ethnicFactionResistance = 0.0f;
    float kinAlliancePreference = 0.0f;
    float mainstreamDetectionRisk = 0.0f;
    float individualisticLoyalty = 0.0f;
};

struct CulturalCompetencyProfile {
    float inGroupNegotiation = 0.0f;
    float outGroupNegotiation = 0.0f;
    float crossEthnicPenalty = 0.0f;
    float languageAccess = 0.0f;
    float translateBonus = 0.0f;
};

struct OpportunityPathProfile {
    float streetCrimePath = 0.0f;
    float organizerPath = 0.0f;
    float institutionalPath = 0.0f;
    float corporatePath = 0.0f;
};

struct PlayerProfile {
    CharacterDraft draft;
    NetworkAccessProfile networkAccess;
    LegitimacyProfile legitimacy;
    LoyaltyBiasProfile loyaltyBias;
    CulturalCompetencyProfile culturalCompetency;
    OpportunityPathProfile opportunityPaths;
};

} // namespace Core
