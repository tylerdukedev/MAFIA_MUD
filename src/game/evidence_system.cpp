#include "game/evidence_system.h"
#include <algorithm>
#include <cstring>

namespace Core {

void resetEvidenceSystemStore(EvidenceSystemStore& store) {
    for (int32_t caseIndex = 0; caseIndex < MAX_INVESTIGATION_CASES; ++caseIndex) {
        store.bundles[caseIndex] = CaseEvidenceBundle{};
    }
}

int32_t computeCaseEvidenceScore(const EvidenceSystemStore& store, int32_t caseIndex) {
    if (caseIndex < 0 || caseIndex >= MAX_INVESTIGATION_CASES) {
        return 0;
    }
    const CaseEvidenceBundle& bundle = store.bundles[caseIndex];
    int32_t totalWeight = 0;
    for (int32_t itemIndex = 0; itemIndex < bundle.itemCount; ++itemIndex) {
        const EvidenceItem& item = bundle.items[itemIndex];
        if (item.isActive == 0) {
            continue;
        }
        totalWeight += static_cast<int32_t>(item.weight);
    }
    return std::clamp(totalWeight, 0, 100);
}

int32_t computeAggregateEvidenceScore(const EvidenceSystemStore& store, const InvestigationCaseStore& caseStore) {
    int32_t bestScore = 0;
    for (int32_t caseIndex = 0; caseIndex < MAX_INVESTIGATION_CASES; ++caseIndex) {
        const InvestigationCase& investigationCase = caseStore.cases[caseIndex];
        if (investigationCase.isActive == 0) {
            continue;
        }
        const int32_t caseScore = computeCaseEvidenceScore(store, caseIndex);
        bestScore = std::max(bestScore, caseScore);
    }
    return bestScore;
}

bool tryAddEvidenceToCase(
    EvidenceSystemStore& store,
    InvestigationCaseStore& caseStore,
    int32_t caseIndex,
    EvidenceKind kind,
    int32_t weight,
    const char* description,
    uint64_t tickCount) {
    if (caseIndex < 0 || caseIndex >= MAX_INVESTIGATION_CASES) {
        return false;
    }
    InvestigationCase& investigationCase = caseStore.cases[caseIndex];
    if (investigationCase.isActive == 0) {
        return false;
    }
    CaseEvidenceBundle& bundle = store.bundles[caseIndex];
    if (bundle.itemCount >= MAX_EVIDENCE_PER_CASE) {
        return false;
    }
    EvidenceItem& item = bundle.items[bundle.itemCount];
    item.kind = kind;
    item.weight = static_cast<uint8_t>(std::clamp(weight, 1, 30));
    item.isActive = 1;
    if (description != nullptr) {
        std::strncpy(item.description, description, sizeof(item.description) - 1);
    }
    bundle.itemCount += 1;
    addCaseCertainty(investigationCase, static_cast<int32_t>(item.weight));
    investigationCase.lastActivityTick = tickCount;
    return true;
}

const EvidenceItem* getCaseEvidenceItem(const EvidenceSystemStore& store, int32_t caseIndex, int32_t itemIndex) {
    if (caseIndex < 0 || caseIndex >= MAX_INVESTIGATION_CASES) {
        return nullptr;
    }
    const CaseEvidenceBundle& bundle = store.bundles[caseIndex];
    if (itemIndex < 0 || itemIndex >= bundle.itemCount) {
        return nullptr;
    }
    return &bundle.items[itemIndex];
}

const char* evidenceKindToString(EvidenceKind kind) {
    switch (kind) {
    case EvidenceKind::Witness:
        return "Witness";
    case EvidenceKind::Financial:
        return "Financial";
    default:
        return "Physical";
    }
}

} // namespace Core
