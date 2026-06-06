#include "game/crime_scene_generator.h"
#include "game/evidence_system.h"
#include "game/investigation_case_store.h"
#include "game/player_law_enforcement.h"
#include "game/street_crime.h"
#include <catch2/catch_test_macros.hpp>

using namespace Core;

TEST_CASE("Open investigation case tracks certainty and officers", "[investigation]") {
    InvestigationCaseStore caseStore{};
    resetInvestigationCaseStore(caseStore);
    const int32_t caseIndex = openInvestigationCase(caseStore, CrimeLegalTier::Street, 10ULL, "Pickpocket sweep");
    REQUIRE(caseIndex >= 0);
    InvestigationCase& investigationCase = caseStore.cases[caseIndex];
    addCaseOfficer(investigationCase, 2);
    addCaseOfficer(investigationCase, 3);
    addCaseOfficer(investigationCase, 4);
    addCaseOfficer(investigationCase, 5);
    addCaseOfficer(investigationCase, 6);
    REQUIRE(investigationCase.officerCount == MAX_CASE_OFFICERS);
    addCaseCertainty(investigationCase, 92);
    REQUIRE(tryIssueCaseWarrant(investigationCase));
    REQUIRE(static_cast<InvestigationCaseStatus>(investigationCase.status) == InvestigationCaseStatus::WarrantIssued);
}

TEST_CASE("Evidence rollup syncs law enforcement evidence score", "[investigation]") {
    InvestigationCaseStore caseStore{};
    EvidenceSystemStore evidenceStore{};
    PlayerLawEnforcementStore lawStore{};
    resetInvestigationCaseStore(caseStore);
    resetEvidenceSystemStore(evidenceStore);
    resetPlayerLawEnforcementStore(lawStore);
    const int32_t caseIndex = openInvestigationCase(caseStore, CrimeLegalTier::Organization, 20ULL, "Warehouse pinch");
    REQUIRE(tryAddEvidenceToCase(
        evidenceStore,
        caseStore,
        caseIndex,
        EvidenceKind::Physical,
        EVIDENCE_WEIGHT_PHYSICAL,
        "Shell casing",
        21ULL));
    syncLawEnforcementEvidenceRollup(lawStore, caseStore, evidenceStore);
    REQUIRE(lawStore.evidenceScore >= EVIDENCE_WEIGHT_PHYSICAL);
}

TEST_CASE("Street crime success can leave crime scene evidence", "[investigation]") {
    InvestigationCaseStore caseStore{};
    EvidenceSystemStore evidenceStore{};
    resetInvestigationCaseStore(caseStore);
    resetEvidenceSystemStore(evidenceStore);
    const StreetCrimeDefinition* crime = getStreetCrimeDefinition(0);
    REQUIRE(crime != nullptr);
    bool foundEvidence = false;
    for (uint64_t tick = 0ULL; tick < 500ULL; ++tick) {
        if (tryGenerateCrimeSceneEvidence(evidenceStore, caseStore, *crime, 0, 4242ULL, tick)) {
            foundEvidence = true;
            break;
        }
    }
    REQUIRE(foundEvidence);
    REQUIRE(caseStore.activeCount >= 1);
}

TEST_CASE("Warrant issues at ninety percent case certainty", "[investigation]") {
    InvestigationCaseStore caseStore{};
    resetInvestigationCaseStore(caseStore);
    const int32_t caseIndex = openInvestigationCase(caseStore, CrimeLegalTier::Street, 1ULL, "Mugging");
    InvestigationCase& investigationCase = caseStore.cases[caseIndex];
    addCaseCertainty(investigationCase, INVESTIGATION_WARRANT_CERTAINTY - 1);
    REQUIRE_FALSE(tryIssueCaseWarrant(investigationCase));
    addCaseCertainty(investigationCase, 1);
    REQUIRE(tryIssueCaseWarrant(investigationCase));
    REQUIRE(static_cast<InvestigationCaseStatus>(investigationCase.status) == InvestigationCaseStatus::WarrantIssued);
}
