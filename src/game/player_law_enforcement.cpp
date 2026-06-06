#include "game/player_law_enforcement.h"
#include "game/evidence_system.h"
#include "game/investigation_case_store.h"
#include "game/player_organization.h"
#include "game/player_operations.h"
#include <algorithm>
#include <cstdio>
#include <cstring>

namespace Core {

void resetPlayerLawEnforcementStore(PlayerLawEnforcementStore& store) {
    store.personalHeat = 0;
    store.investigationTier = 0;
    store.evidenceScore = 0;
    store.witnessCount = 0;
    store.activeWarrantCount = 0;
    store.lastHeatDecayTick = 0;
    store.lastEvidenceDecayTick = 0;
    store.lastWitnessLabel[0] = '\0';
}

void addPlayerHeat(PlayerLawEnforcementStore& store, int32_t heatDelta) {
    if (heatDelta <= 0) {
        return;
    }
    store.personalHeat = std::clamp(store.personalHeat + heatDelta, PLAYER_HEAT_MIN, PLAYER_HEAT_MAX);
    refreshInvestigationTier(store);
}

void addPoliceEvidence(PlayerLawEnforcementStore& store, int32_t evidenceDelta) {
    if (evidenceDelta <= 0) {
        return;
    }
    store.evidenceScore = std::clamp(store.evidenceScore + evidenceDelta, 0, 100);
}

void refreshInvestigationTier(PlayerLawEnforcementStore& store) {
    if (store.personalHeat >= POLICE_INVESTIGATION_TIER3_HEAT || store.activeWarrantCount > 0) {
        store.investigationTier = 3;
        return;
    }
    if (store.personalHeat >= POLICE_INVESTIGATION_TIER2_HEAT) {
        store.investigationTier = 2;
        return;
    }
    if (store.personalHeat >= POLICE_INVESTIGATION_TIER1_HEAT) {
        store.investigationTier = 1;
        return;
    }
    store.investigationTier = 0;
}

int32_t computePlayerStreetNotoriety(
    const PlayerLawEnforcementStore& lawStore,
    const PlayerOrganizationStore& organizationStore,
    const PlayerProfile& profile,
    const PlayerWallet& wallet) {
    int32_t notoriety = lawStore.personalHeat / 3;
    notoriety += getPlayerReputationScore(profile) / 5;
    notoriety += static_cast<int32_t>(wallet.lifetimeCrimeCents / 400);
    if (organizationStore.powerTier == PlayerPowerTier::Crew) {
        notoriety += 6;
    }
    if (organizationStore.powerTier == PlayerPowerTier::Organization) {
        notoriety += 12;
    }
    return std::clamp(notoriety, 0, 45);
}

bool tryIssueWarrantIfThresholdMet(PlayerLawEnforcementStore& store) {
    if (store.activeWarrantCount >= POLICE_MAX_ACTIVE_WARRANTS) {
        return false;
    }
    if (store.evidenceScore < POLICE_EVIDENCE_WARRANT_THRESHOLD) {
        return false;
    }
    if (store.personalHeat < POLICE_HEAT_WARRANT_THRESHOLD) {
        return false;
    }
    store.activeWarrantCount += 1;
    store.evidenceScore = std::clamp(store.evidenceScore + 6, 0, 100);
    refreshInvestigationTier(store);
    return true;
}

void rollStreetCrimeWitness(
    PlayerLawEnforcementStore& store,
    const PlayerOrganizationStore& organizationStore,
    const PlayerProfile& profile,
    const PlayerWallet& wallet,
    uint64_t worldSeed,
    uint64_t tickCount,
    int32_t crimeIndex,
    bool crimeSucceeded) {
    const int32_t notoriety = computePlayerStreetNotoriety(store, organizationStore, profile, wallet);
    int32_t witnessChance = POLICE_WITNESS_BASE_PERCENT + (notoriety * POLICE_WITNESS_NOTORIETY_SCALE_PERCENT) / 45;
    if (!crimeSucceeded) {
        witnessChance += POLICE_WITNESS_FAIL_BONUS_PERCENT;
    }
    witnessChance = std::clamp(witnessChance, 6, 72);
    const uint32_t roll = static_cast<uint32_t>((worldSeed ^ tickCount ^ static_cast<uint64_t>(crimeIndex)) % 100ULL);
    if (static_cast<int32_t>(roll) >= witnessChance) {
        return;
    }
    store.witnessCount += 1;
    const int32_t evidenceGain = crimeSucceeded ? 6 : 12;
    addPoliceEvidence(store, evidenceGain);
    std::snprintf(store.lastWitnessLabel, sizeof(store.lastWitnessLabel), "%s", "Neighborhood witness");
    tryIssueWarrantIfThresholdMet(store);
}

void tickPoliceEvidenceDecay(PlayerLawEnforcementStore& store, uint64_t tickCount) {
    if (store.evidenceScore <= 0) {
        return;
    }
    if (store.lastEvidenceDecayTick != 0ULL
        && tickCount - store.lastEvidenceDecayTick < static_cast<uint64_t>(POLICE_EVIDENCE_DECAY_INTERVAL_TICKS)) {
        return;
    }
    store.lastEvidenceDecayTick = tickCount;
    store.evidenceScore = std::max(0, store.evidenceScore - 1);
    if (store.personalHeat < POLICE_INVESTIGATION_TIER1_HEAT && store.evidenceScore < POLICE_EVIDENCE_WARRANT_THRESHOLD / 2) {
        store.activeWarrantCount = 0;
    }
    refreshInvestigationTier(store);
}

void tickPoliceWarrantReview(PlayerLawEnforcementStore& store, uint64_t tickCount) {
    (void)tickCount;
    if (store.activeWarrantCount <= 0) {
        return;
    }
    if (store.personalHeat <= 12 && store.evidenceScore <= 10) {
        store.activeWarrantCount = 0;
        refreshInvestigationTier(store);
    }
}

const char* getPoliceInvestigationLabel(int32_t investigationTier) {
    switch (investigationTier) {
    case 3:
        return "Active case / warrant";
    case 2:
        return "Under watch";
    case 1:
        return "On the beat's notes";
    default:
        return "Off the radar";
    }
}

bool hasActivePoliceWarrant(const PlayerLawEnforcementStore& store) {
    return store.activeWarrantCount > 0;
}

void syncLawEnforcementEvidenceRollup(
    PlayerLawEnforcementStore& store,
    const InvestigationCaseStore& caseStore,
    const EvidenceSystemStore& evidenceStore) {
    const int32_t caseCertainty = computeAggregateCaseCertainty(caseStore);
    const int32_t evidenceTotal = computeAggregateEvidenceScore(evidenceStore, caseStore);
    store.evidenceScore = std::clamp(std::max(caseCertainty, evidenceTotal), 0, PLAYER_HEAT_MAX);
    refreshInvestigationTier(store);
}

void dispatchPoliceOnCrime(
    InvestigationCaseStore& caseStore,
    CrimeLegalTier legalTier,
    uint64_t tickCount,
    const char* dispatchLabel) {
    int32_t caseIndex = findOpenInvestigationCase(caseStore, legalTier);
    if (caseIndex < 0) {
        caseIndex = openInvestigationCase(caseStore, legalTier, tickCount, dispatchLabel);
    }
    if (caseIndex < 0) {
        return;
    }
    InvestigationCase& investigationCase = caseStore.cases[caseIndex];
    investigationCase.dispatchPending = 1;
    investigationCase.lastActivityTick = tickCount;
    if (dispatchLabel != nullptr && investigationCase.caseLabel[0] == '\0') {
        std::strncpy(investigationCase.caseLabel, dispatchLabel, sizeof(investigationCase.caseLabel) - 1);
    }
    if (caseStore.primaryCaseIndex < 0) {
        caseStore.primaryCaseIndex = caseIndex;
    }
}

} // namespace Core
