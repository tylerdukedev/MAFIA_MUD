#include "sim/police_system.h"
#include "game/evidence_system.h"
#include "game/investigation_case_store.h"
#include <algorithm>

namespace Core {

namespace {

void processInvestigationDispatches(
    InvestigationCaseStore& caseStore,
    PlayerLawEnforcementStore& lawStore,
    uint64_t tickCount) {
    for (int32_t caseIndex = 0; caseIndex < MAX_INVESTIGATION_CASES; ++caseIndex) {
        InvestigationCase& investigationCase = caseStore.cases[caseIndex];
        if (investigationCase.isActive == 0 || investigationCase.dispatchPending == 0) {
            continue;
        }
        investigationCase.dispatchPending = 0;
        addCaseCertainty(investigationCase, 4);
        investigationCase.lastActivityTick = tickCount;
        lawStore.personalHeat = std::clamp(lawStore.personalHeat + 2, PLAYER_HEAT_MIN, PLAYER_HEAT_MAX);
    }
}

void reviewInvestigationWarrants(
    InvestigationCaseStore& caseStore,
    PlayerLawEnforcementStore& lawStore) {
    for (int32_t caseIndex = 0; caseIndex < MAX_INVESTIGATION_CASES; ++caseIndex) {
        InvestigationCase& investigationCase = caseStore.cases[caseIndex];
        if (investigationCase.isActive == 0) {
            continue;
        }
        if (!tryIssueCaseWarrant(investigationCase)) {
            continue;
        }
        if (lawStore.activeWarrantCount < POLICE_MAX_ACTIVE_WARRANTS) {
            lawStore.activeWarrantCount += 1;
        }
        refreshInvestigationTier(lawStore);
    }
}

} // namespace

void PoliceSystem::bind(
    const SimWorldBindings& inputBindings,
    PlayerLawEnforcementStore* inputLawStore,
    InvestigationCaseStore* inputCaseStore,
    EvidenceSystemStore* inputEvidenceStore) {
    bindings = inputBindings;
    lawStore = inputLawStore;
    caseStore = inputCaseStore;
    evidenceStore = inputEvidenceStore;
}

const char* PoliceSystem::getName() const {
    return "PoliceSystem";
}

void PoliceSystem::onTick(uint64_t tickCount) {
    lastTickCount = tickCount;
    if (lawStore == nullptr || bindings.playerProfile == nullptr) {
        return;
    }
    if (caseStore != nullptr) {
        if (evidenceStore != nullptr) {
            processInvestigationDispatches(*caseStore, *lawStore, tickCount);
            reviewInvestigationWarrants(*caseStore, *lawStore);
            syncLawEnforcementEvidenceRollup(*lawStore, *caseStore, *evidenceStore);
        }
    }
    tickPoliceEvidenceDecay(*lawStore, tickCount);
    tickPoliceWarrantReview(*lawStore, tickCount);
    if (lawStore->personalHeat <= 0) {
        return;
    }
    if (lawStore->lastHeatDecayTick != 0ULL
        && tickCount - lawStore->lastHeatDecayTick < static_cast<uint64_t>(POLICE_HEAT_DECAY_INTERVAL_TICKS)) {
        return;
    }
    lawStore->lastHeatDecayTick = tickCount;
    const float decayScale = 1.0f + bindings.playerProfile->legitimacy.policeAttentionDecay;
    const int32_t decayAmount = std::max(1, static_cast<int32_t>(decayScale));
    lawStore->personalHeat = std::max(PLAYER_HEAT_MIN, lawStore->personalHeat - decayAmount);
    refreshInvestigationTier(*lawStore);
}

} // namespace Core
