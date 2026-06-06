#include "game/trial_system.h"
#include "game/ny_penal_codes.h"
#include "utils/seed_hash.h"
#include <algorithm>
#include <cstdio>

namespace Core {

TrialMode selectTrialMode(CrimeLegalTier legalTier, int32_t evidenceScore) {
    if (static_cast<int32_t>(legalTier) >= static_cast<int32_t>(CrimeLegalTier::Organization) || evidenceScore >= 55) {
        return TrialMode::Jury;
    }
    return TrialMode::Bench;
}

TrialDocketEntry buildTrialDocket(
    CrimeLegalTier chargeTier,
    const InvestigationCaseStore& caseStore,
    const EvidenceSystemStore& evidenceStore,
    int32_t caseIndex,
    const PlayerLegalCounselStore& legalCounselStore) {
    TrialDocketEntry entry{};
    entry.chargeTier = chargeTier;
    const InvestigationCase* investigationCase = getInvestigationCase(caseStore, caseIndex);
    entry.evidenceScore = computeCaseEvidenceScore(evidenceStore, caseIndex);
    if (investigationCase != nullptr) {
        entry.evidenceScore = std::max(entry.evidenceScore, static_cast<int32_t>(investigationCase->certainty));
    }
    entry.mode = selectTrialMode(chargeTier, entry.evidenceScore);
    int32_t acquittalChance = 28;
    acquittalChance -= entry.evidenceScore / 3;
    acquittalChance += rollCourtAcquittalBonusPercent(legalCounselStore, 0ULL, 0ULL);
    if (entry.mode == TrialMode::Jury) {
        acquittalChance -= 6;
    }
    entry.acquittalChancePercent = std::clamp(acquittalChance, 4, 72);
    std::snprintf(
        entry.docketLabel,
        sizeof(entry.docketLabel),
        "%s trial — %s",
        trialModeToString(entry.mode),
        getNyPenalShortTitle(chargeTier));
    return entry;
}

CourtOutcome resolveTrialOutcome(
    const TrialDocketEntry& docket,
    const PlayerLegalCounselStore& legalCounselStore,
    const PlayerLawEnforcementStore& lawStore,
    uint64_t worldSeed,
    uint64_t tickCount) {
    const int32_t acquittalBonus = rollCourtAcquittalBonusPercent(legalCounselStore, worldSeed, tickCount);
    const int32_t effectiveEvidence = std::max(docket.evidenceScore, lawStore.evidenceScore);
    const uint32_t roll = Utils::hashSeedMix(worldSeed, static_cast<int32_t>(tickCount), 0x54524941) % 100U;
    const int32_t acquittalThreshold = docket.acquittalChancePercent + acquittalBonus;
    if (effectiveEvidence < 20 && static_cast<int32_t>(roll) < acquittalThreshold) {
        return CourtOutcome::Acquitted;
    }
    if (effectiveEvidence < 45 && static_cast<int32_t>(roll) < acquittalThreshold - 8) {
        return CourtOutcome::Probation;
    }
    if (static_cast<int32_t>(docket.chargeTier) >= static_cast<int32_t>(CrimeLegalTier::Organization) && roll < 82U) {
        return CourtOutcome::Prison;
    }
    if (roll < 40U) {
        return CourtOutcome::Probation;
    }
    return CourtOutcome::Prison;
}

const char* trialModeToString(TrialMode mode) {
    switch (mode) {
    case TrialMode::Jury:
        return "Jury";
    default:
        return "Bench";
    }
}

} // namespace Core
