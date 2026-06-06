#include "game/plea_bargain.h"
#include "game/player_criminal_justice.h"
#include "utils/seed_hash.h"
#include <algorithm>
#include <cstdio>

namespace Core {

PleaBargainOffer computePleaBargainOffer(
    CrimeLegalTier chargeTier,
    const InvestigationCaseStore& caseStore,
    const EvidenceSystemStore& evidenceStore,
    int32_t caseIndex,
    uint64_t worldSeed,
    uint64_t tickCount) {
    PleaBargainOffer offer{};
    const InvestigationCase* investigationCase = getInvestigationCase(caseStore, caseIndex);
    int32_t evidenceScore = computeCaseEvidenceScore(evidenceStore, caseIndex);
    if (investigationCase != nullptr) {
        evidenceScore = std::max(evidenceScore, static_cast<int32_t>(investigationCase->certainty));
    }
    offer.evidenceScoreAtOffer = evidenceScore;
    if (evidenceScore < 18) {
        offer.dealTier = PleaDealTier::Dismissal;
        offer.reducedTier = CrimeLegalTier::PettyStreet;
        std::snprintf(offer.offerSummary, sizeof(offer.offerSummary), "%s", "DA offers dismissal — thin file");
        return offer;
    }
    if (evidenceScore < 40) {
        offer.dealTier = PleaDealTier::ReducedMisdemeanor;
        offer.reducedTier = CrimeLegalTier::Street;
        offer.recommendedProbationTicks = JUSTICE_PROBATION_TICKS / 2;
        std::snprintf(offer.offerSummary, sizeof(offer.offerSummary), "%s", "Reduced charge with probation");
        return offer;
    }
    if (evidenceScore < 70) {
        offer.dealTier = PleaDealTier::StandardPlea;
        offer.reducedTier = chargeTier;
        offer.recommendedProbationTicks = JUSTICE_PROBATION_TICKS;
        std::snprintf(offer.offerSummary, sizeof(offer.offerSummary), "%s", "Standard plea — probation recommended");
        return offer;
    }
    offer.dealTier = PleaDealTier::HarshPlea;
    offer.reducedTier = chargeTier;
    offer.recommendedPrisonTicks = JUSTICE_PRISON_BASE_TICKS + static_cast<int32_t>(chargeTier) * JUSTICE_PRISON_PER_LEGAL_TIER_TICKS;
    const uint32_t roll = Utils::hashSeedMix(worldSeed, static_cast<int32_t>(tickCount), 0x504C4541) % 20U;
    offer.recommendedPrisonTicks = offer.recommendedPrisonTicks * static_cast<int32_t>(80U + roll) / 100;
    std::snprintf(offer.offerSummary, sizeof(offer.offerSummary), "%s", "Harsh plea — state time on the table");
    return offer;
}

const char* pleaDealTierToString(PleaDealTier dealTier) {
    switch (dealTier) {
    case PleaDealTier::Dismissal:
        return "Dismissal";
    case PleaDealTier::ReducedMisdemeanor:
        return "Reduced misdemeanor";
    case PleaDealTier::StandardPlea:
        return "Standard plea";
    case PleaDealTier::HarshPlea:
        return "Harsh plea";
    default:
        return "No offer";
    }
}

} // namespace Core
