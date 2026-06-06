#include "game/evidence_system.h"
#include "game/investigation_case_store.h"
#include "game/legal_counsel.h"
#include "game/ny_penal_codes.h"
#include "game/plea_bargain.h"
#include "game/player_criminal_justice.h"
#include "game/player_law_enforcement.h"
#include "game/trial_system.h"
#include <catch2/catch_test_macros.hpp>
#include <cstring>

using namespace Core;

TEST_CASE("NY penal codes map legal tiers to statutes", "[plea_trial]") {
    REQUIRE(std::strcmp(getNyPenalStatuteCode(CrimeLegalTier::Street), "PL 155.25") == 0);
    REQUIRE(std::strcmp(getNyPenalShortTitle(CrimeLegalTier::Financial), "Scheme to Defraud") == 0);
}

TEST_CASE("Weak evidence produces dismissal plea offer", "[plea_trial]") {
    InvestigationCaseStore caseStore{};
    EvidenceSystemStore evidenceStore{};
    resetInvestigationCaseStore(caseStore);
    resetEvidenceSystemStore(evidenceStore);
    const int32_t caseIndex = openInvestigationCase(caseStore, CrimeLegalTier::Street, 5ULL, "Thin file");
    const PleaBargainOffer offer = computePleaBargainOffer(
        CrimeLegalTier::Street,
        caseStore,
        evidenceStore,
        caseIndex,
        99ULL,
        6ULL);
    REQUIRE(offer.dealTier == PleaDealTier::Dismissal);
}

TEST_CASE("Strong evidence produces harsh plea offer", "[plea_trial]") {
    InvestigationCaseStore caseStore{};
    EvidenceSystemStore evidenceStore{};
    resetInvestigationCaseStore(caseStore);
    resetEvidenceSystemStore(evidenceStore);
    const int32_t caseIndex = openInvestigationCase(caseStore, CrimeLegalTier::Organization, 8ULL, "Racket file");
    InvestigationCase& investigationCase = caseStore.cases[caseIndex];
    addCaseCertainty(investigationCase, 78);
    tryAddEvidenceToCase(
        evidenceStore,
        caseStore,
        caseIndex,
        EvidenceKind::Financial,
        EVIDENCE_WEIGHT_FINANCIAL,
        "Ledger",
        9ULL);
    const PleaBargainOffer offer = computePleaBargainOffer(
        CrimeLegalTier::Organization,
        caseStore,
        evidenceStore,
        caseIndex,
        101ULL,
        10ULL);
    REQUIRE(offer.dealTier == PleaDealTier::HarshPlea);
    REQUIRE(offer.recommendedPrisonTicks > 0);
}

TEST_CASE("Trial docket selects jury for organization charges", "[plea_trial]") {
    InvestigationCaseStore caseStore{};
    EvidenceSystemStore evidenceStore{};
    PlayerLegalCounselStore counselStore{};
    resetInvestigationCaseStore(caseStore);
    resetEvidenceSystemStore(evidenceStore);
    const int32_t caseIndex = openInvestigationCase(caseStore, CrimeLegalTier::Organization, 12ULL, "Enterprise case");
    InvestigationCase& investigationCase = caseStore.cases[caseIndex];
    addCaseCertainty(investigationCase, 60);
    const TrialDocketEntry docket = buildTrialDocket(
        CrimeLegalTier::Organization,
        caseStore,
        evidenceStore,
        caseIndex,
        counselStore);
    REQUIRE(docket.mode == TrialMode::Jury);
    REQUIRE(docket.evidenceScore >= 60);
}

TEST_CASE("Trial resolution returns a court outcome", "[plea_trial]") {
    PlayerLawEnforcementStore lawStore{};
    PlayerLegalCounselStore counselStore{};
    TrialDocketEntry docket{};
    docket.chargeTier = CrimeLegalTier::Street;
    docket.evidenceScore = 30;
    docket.acquittalChancePercent = 25;
    const CourtOutcome outcome = resolveTrialOutcome(docket, counselStore, lawStore, 555ULL, 77ULL);
    REQUIRE(outcome != CourtOutcome::None);
}
