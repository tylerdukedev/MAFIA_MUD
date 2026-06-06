#include "game/investigation_case_store.h"
#include <algorithm>
#include <cstring>

namespace Core {

void resetInvestigationCaseStore(InvestigationCaseStore& store) {
    for (int32_t caseIndex = 0; caseIndex < MAX_INVESTIGATION_CASES; ++caseIndex) {
        store.cases[caseIndex] = InvestigationCase{};
    }
    store.activeCount = 0;
    store.primaryCaseIndex = -1;
}

int32_t findOpenInvestigationCase(const InvestigationCaseStore& store, CrimeLegalTier legalTier) {
    for (int32_t caseIndex = 0; caseIndex < MAX_INVESTIGATION_CASES; ++caseIndex) {
        const InvestigationCase& investigationCase = store.cases[caseIndex];
        if (investigationCase.isActive == 0) {
            continue;
        }
        const auto status = static_cast<InvestigationCaseStatus>(investigationCase.status);
        if (status == InvestigationCaseStatus::Closed) {
            continue;
        }
        if (investigationCase.legalTier == legalTier) {
            return caseIndex;
        }
    }
    return -1;
}

int32_t openInvestigationCase(
    InvestigationCaseStore& store,
    CrimeLegalTier legalTier,
    uint64_t tickCount,
    const char* caseLabel) {
    int32_t openSlot = -1;
    for (int32_t caseIndex = 0; caseIndex < MAX_INVESTIGATION_CASES; ++caseIndex) {
        if (store.cases[caseIndex].isActive == 0) {
            openSlot = caseIndex;
            break;
        }
    }
    if (openSlot < 0) {
        return -1;
    }
    InvestigationCase& investigationCase = store.cases[openSlot];
    investigationCase = InvestigationCase{};
    investigationCase.isActive = 1;
    investigationCase.legalTier = legalTier;
    investigationCase.status = static_cast<uint8_t>(InvestigationCaseStatus::Open);
    investigationCase.openedTick = tickCount;
    investigationCase.lastActivityTick = tickCount;
    investigationCase.dispatchPending = 1;
    if (caseLabel != nullptr) {
        std::strncpy(investigationCase.caseLabel, caseLabel, sizeof(investigationCase.caseLabel) - 1);
    }
    store.activeCount += 1;
    if (store.primaryCaseIndex < 0) {
        store.primaryCaseIndex = openSlot;
    }
    return openSlot;
}

void addCaseOfficer(InvestigationCase& investigationCase, uint8_t contactIndex) {
    if (investigationCase.officerCount >= MAX_CASE_OFFICERS) {
        return;
    }
    for (int32_t officerIndex = 0; officerIndex < investigationCase.officerCount; ++officerIndex) {
        if (investigationCase.officerContactIndices[officerIndex] == contactIndex) {
            return;
        }
    }
    investigationCase.officerContactIndices[investigationCase.officerCount] = contactIndex;
    investigationCase.officerCount += 1;
}

void addCaseCertainty(InvestigationCase& investigationCase, int32_t certaintyDelta) {
    if (certaintyDelta <= 0) {
        return;
    }
    const int32_t updated = static_cast<int32_t>(investigationCase.certainty) + certaintyDelta;
    investigationCase.certainty = static_cast<uint8_t>(std::clamp(updated, 0, 100));
}

bool tryIssueCaseWarrant(InvestigationCase& investigationCase) {
    if (investigationCase.isActive == 0) {
        return false;
    }
    const auto status = static_cast<InvestigationCaseStatus>(investigationCase.status);
    if (status == InvestigationCaseStatus::WarrantIssued || status == InvestigationCaseStatus::Closed) {
        return false;
    }
    if (static_cast<int32_t>(investigationCase.certainty) < INVESTIGATION_WARRANT_CERTAINTY) {
        return false;
    }
    investigationCase.status = static_cast<uint8_t>(InvestigationCaseStatus::WarrantIssued);
    return true;
}

int32_t computeAggregateCaseCertainty(const InvestigationCaseStore& store) {
    int32_t bestCertainty = 0;
    for (int32_t caseIndex = 0; caseIndex < MAX_INVESTIGATION_CASES; ++caseIndex) {
        const InvestigationCase& investigationCase = store.cases[caseIndex];
        if (investigationCase.isActive == 0) {
            continue;
        }
        const auto status = static_cast<InvestigationCaseStatus>(investigationCase.status);
        if (status == InvestigationCaseStatus::Closed) {
            continue;
        }
        bestCertainty = std::max(bestCertainty, static_cast<int32_t>(investigationCase.certainty));
    }
    return bestCertainty;
}

const InvestigationCase* getPrimaryInvestigationCase(const InvestigationCaseStore& store) {
    if (store.primaryCaseIndex < 0 || store.primaryCaseIndex >= MAX_INVESTIGATION_CASES) {
        return nullptr;
    }
    const InvestigationCase& investigationCase = store.cases[store.primaryCaseIndex];
    if (investigationCase.isActive == 0) {
        return nullptr;
    }
    return &investigationCase;
}

const InvestigationCase* getInvestigationCase(const InvestigationCaseStore& store, int32_t caseIndex) {
    if (caseIndex < 0 || caseIndex >= MAX_INVESTIGATION_CASES) {
        return nullptr;
    }
    const InvestigationCase& investigationCase = store.cases[caseIndex];
    if (investigationCase.isActive == 0) {
        return nullptr;
    }
    return &investigationCase;
}

const char* investigationCaseStatusToString(InvestigationCaseStatus status) {
    switch (status) {
    case InvestigationCaseStatus::WarrantIssued:
        return "Warrant issued";
    case InvestigationCaseStatus::Closed:
        return "Closed";
    default:
        return "Open investigation";
    }
}

} // namespace Core
