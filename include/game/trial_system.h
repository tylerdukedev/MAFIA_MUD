#pragma once

#include "game/crime_legal_tier.h"
#include "game/evidence_system.h"
#include "game/investigation_case_store.h"
#include "game/legal_counsel.h"
#include "game/player_criminal_justice.h"
#include "game/player_law_enforcement.h"
#include <cstdint>

namespace Core {

enum class TrialMode : uint8_t {
    Bench = 0,
    Jury = 1,
};

struct TrialDocketEntry {
    TrialMode mode = TrialMode::Bench;
    CrimeLegalTier chargeTier = CrimeLegalTier::Street;
    int32_t evidenceScore = 0;
    int32_t acquittalChancePercent = 0;
    char docketLabel[64]{};
};

TrialMode selectTrialMode(CrimeLegalTier legalTier, int32_t evidenceScore);
TrialDocketEntry buildTrialDocket(
    CrimeLegalTier chargeTier,
    const InvestigationCaseStore& caseStore,
    const EvidenceSystemStore& evidenceStore,
    int32_t caseIndex,
    const PlayerLegalCounselStore& legalCounselStore);
CourtOutcome resolveTrialOutcome(
    const TrialDocketEntry& docket,
    const PlayerLegalCounselStore& legalCounselStore,
    const PlayerLawEnforcementStore& lawStore,
    uint64_t worldSeed,
    uint64_t tickCount);
const char* trialModeToString(TrialMode mode);

} // namespace Core
