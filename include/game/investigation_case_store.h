#pragma once

#include "game/crime_legal_tier.h"
#include <cstdint>

namespace Core {

constexpr int32_t MAX_INVESTIGATION_CASES = 32;
constexpr int32_t MAX_CASE_OFFICERS = 4;
constexpr int32_t INVESTIGATION_WARRANT_CERTAINTY = 90;

enum class InvestigationCaseStatus : uint8_t {
    Open = 0,
    WarrantIssued = 1,
    Closed = 2,
};

struct InvestigationCase {
    CrimeLegalTier legalTier = CrimeLegalTier::Street;
    uint8_t certainty = 0;
    uint8_t status = 0;
    uint8_t officerCount = 0;
    uint8_t officerContactIndices[MAX_CASE_OFFICERS]{};
    char caseLabel[48]{};
    uint64_t openedTick = 0;
    uint64_t lastActivityTick = 0;
    uint8_t isActive = 0;
    uint8_t dispatchPending = 0;
};

struct InvestigationCaseStore {
    InvestigationCase cases[MAX_INVESTIGATION_CASES]{};
    int32_t activeCount = 0;
    int32_t primaryCaseIndex = -1;
};

void resetInvestigationCaseStore(InvestigationCaseStore& store);
int32_t findOpenInvestigationCase(const InvestigationCaseStore& store, CrimeLegalTier legalTier);
int32_t openInvestigationCase(
    InvestigationCaseStore& store,
    CrimeLegalTier legalTier,
    uint64_t tickCount,
    const char* caseLabel);
void addCaseOfficer(InvestigationCase& investigationCase, uint8_t contactIndex);
void addCaseCertainty(InvestigationCase& investigationCase, int32_t certaintyDelta);
bool tryIssueCaseWarrant(InvestigationCase& investigationCase);
int32_t computeAggregateCaseCertainty(const InvestigationCaseStore& store);
const InvestigationCase* getPrimaryInvestigationCase(const InvestigationCaseStore& store);
const InvestigationCase* getInvestigationCase(const InvestigationCaseStore& store, int32_t caseIndex);
const char* investigationCaseStatusToString(InvestigationCaseStatus status);

} // namespace Core
