#pragma once

#include "game/crime_legal_tier.h"
#include "game/evidence_system.h"
#include "game/investigation_case_store.h"
#include "game/street_crime.h"
#include <cstdint>

namespace Core {

constexpr int32_t CRIME_SCENE_BASE_EVIDENCE_CHANCE_PERCENT = 18;

bool tryGenerateCrimeSceneEvidence(
    EvidenceSystemStore& evidenceStore,
    InvestigationCaseStore& caseStore,
    const StreetCrimeDefinition& crime,
    int32_t crimeIndex,
    uint64_t worldSeed,
    uint64_t tickCount);

} // namespace Core
