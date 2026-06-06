#include "game/crime_scene_generator.h"
#include "utils/seed_hash.h"
#include <algorithm>

namespace Core {

namespace {

EvidenceKind rollEvidenceKind(uint64_t worldSeed, int32_t crimeIndex, uint64_t tickCount, int32_t rollSalt) {
    const uint32_t roll = Utils::hashSeedMix(worldSeed, static_cast<int32_t>(tickCount), crimeIndex + rollSalt) % 100U;
    if (roll < 45U) {
        return EvidenceKind::Physical;
    }
    if (roll < 78U) {
        return EvidenceKind::Witness;
    }
    return EvidenceKind::Financial;
}

int32_t evidenceWeightForKind(EvidenceKind kind) {
    switch (kind) {
    case EvidenceKind::Witness:
        return EVIDENCE_WEIGHT_WITNESS;
    case EvidenceKind::Financial:
        return EVIDENCE_WEIGHT_FINANCIAL;
    default:
        return EVIDENCE_WEIGHT_PHYSICAL;
    }
}

const char* evidenceDescriptionForKind(EvidenceKind kind) {
    switch (kind) {
    case EvidenceKind::Witness:
        return "Neighbor saw the job go down";
    case EvidenceKind::Financial:
        return "Paper trail from the payout";
    default:
        return "Trace left at the scene";
    }
}

} // namespace

bool tryGenerateCrimeSceneEvidence(
    EvidenceSystemStore& evidenceStore,
    InvestigationCaseStore& caseStore,
    const StreetCrimeDefinition& crime,
    int32_t crimeIndex,
    uint64_t worldSeed,
    uint64_t tickCount) {
    int32_t caseIndex = findOpenInvestigationCase(caseStore, crime.legalTier);
    if (caseIndex < 0) {
        caseIndex = openInvestigationCase(caseStore, crime.legalTier, tickCount, crime.displayName);
    }
    if (caseIndex < 0) {
        return false;
    }
    int32_t evidenceChance = CRIME_SCENE_BASE_EVIDENCE_CHANCE_PERCENT;
    evidenceChance += crime.executionSkillPercent / 4;
    evidenceChance = std::clamp(evidenceChance, 10, 88);
    const uint32_t roll = Utils::hashSeedMix(worldSeed, static_cast<int32_t>(tickCount), crimeIndex + 0x45564944) % 100U;
    if (static_cast<int32_t>(roll) >= evidenceChance) {
        return false;
    }
    const EvidenceKind kind = rollEvidenceKind(worldSeed, crimeIndex, tickCount, static_cast<int32_t>(roll));
    const int32_t weight = evidenceWeightForKind(kind);
    return tryAddEvidenceToCase(
        evidenceStore,
        caseStore,
        caseIndex,
        kind,
        weight,
        evidenceDescriptionForKind(kind),
        tickCount);
}

} // namespace Core
