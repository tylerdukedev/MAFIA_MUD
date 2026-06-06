#pragma once

#include "game/crime_legal_tier.h"
#include "game/evidence_system.h"
#include "game/investigation_case_store.h"
#include <cstdint>

namespace Core {

enum class PleaDealTier : uint8_t {
    None = 0,
    Dismissal = 1,
    ReducedMisdemeanor = 2,
    StandardPlea = 3,
    HarshPlea = 4,
};

struct PleaBargainOffer {
    PleaDealTier dealTier = PleaDealTier::None;
    CrimeLegalTier reducedTier = CrimeLegalTier::PettyStreet;
    int32_t recommendedProbationTicks = 0;
    int32_t recommendedPrisonTicks = 0;
    int32_t evidenceScoreAtOffer = 0;
    char offerSummary[96]{};
};

PleaBargainOffer computePleaBargainOffer(
    CrimeLegalTier chargeTier,
    const InvestigationCaseStore& caseStore,
    const EvidenceSystemStore& evidenceStore,
    int32_t caseIndex,
    uint64_t worldSeed,
    uint64_t tickCount);
const char* pleaDealTierToString(PleaDealTier dealTier);

} // namespace Core
