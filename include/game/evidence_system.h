#pragma once

#include "game/investigation_case_store.h"
#include <cstdint>

namespace Core {

enum class EvidenceKind : uint8_t {
    Physical = 0,
    Witness = 1,
    Financial = 2,
};

constexpr int32_t MAX_EVIDENCE_PER_CASE = 12;
constexpr int32_t EVIDENCE_WEIGHT_PHYSICAL = 8;
constexpr int32_t EVIDENCE_WEIGHT_WITNESS = 10;
constexpr int32_t EVIDENCE_WEIGHT_FINANCIAL = 12;

struct EvidenceItem {
    EvidenceKind kind = EvidenceKind::Physical;
    uint8_t weight = 0;
    uint8_t isActive = 0;
    char description[48]{};
};

struct CaseEvidenceBundle {
    EvidenceItem items[MAX_EVIDENCE_PER_CASE]{};
    int32_t itemCount = 0;
};

struct EvidenceSystemStore {
    CaseEvidenceBundle bundles[MAX_INVESTIGATION_CASES]{};
};

void resetEvidenceSystemStore(EvidenceSystemStore& store);
int32_t computeCaseEvidenceScore(const EvidenceSystemStore& store, int32_t caseIndex);
int32_t computeAggregateEvidenceScore(const EvidenceSystemStore& store, const InvestigationCaseStore& caseStore);
bool tryAddEvidenceToCase(
    EvidenceSystemStore& store,
    InvestigationCaseStore& caseStore,
    int32_t caseIndex,
    EvidenceKind kind,
    int32_t weight,
    const char* description,
    uint64_t tickCount);
const EvidenceItem* getCaseEvidenceItem(const EvidenceSystemStore& store, int32_t caseIndex, int32_t itemIndex);
const char* evidenceKindToString(EvidenceKind kind);

} // namespace Core
