#include "game/legal_counsel.h"

namespace Core {

namespace {

constexpr LawyerTierDefinition LAWYER_TIERS[] = {
    {LawyerTier::PublicDefender, "Public Defender", 0, 5, 8},
    {LawyerTier::Neighborhood, "Neighborhood Lawyer", 15000, 12, 14},
    {LawyerTier::Experienced, "Experienced Counsel", 45000, 22, 20},
    {LawyerTier::HighProfile, "High-Profile Firm", 120000, 35, 28},
};

} // namespace

void resetPlayerLegalCounselStore(PlayerLegalCounselStore& store) {
    store.hiredLawyerTier = LawyerTier::PublicDefender;
    store.hasRetainer = false;
    store.activeCaseId = 0;
}

const LawyerTierDefinition* getLawyerTierDefinition(LawyerTier tier) {
    const int32_t tierIndex = static_cast<int32_t>(tier);
    if (tierIndex < 0 || tierIndex >= static_cast<int32_t>(sizeof(LAWYER_TIERS) / sizeof(LAWYER_TIERS[0]))) {
        return &LAWYER_TIERS[0];
    }
    return &LAWYER_TIERS[tierIndex];
}

int32_t getLawyerTierCount() {
    return static_cast<int32_t>(sizeof(LAWYER_TIERS) / sizeof(LAWYER_TIERS[0]));
}

bool tryHireLawyer(PlayerLegalCounselStore& store, PlayerWallet& wallet, LawyerTier tier) {
    const LawyerTierDefinition* lawyer = getLawyerTierDefinition(tier);
    if (lawyer->retainerCents > 0 && !tryDebitCash(wallet, lawyer->retainerCents)) {
        return false;
    }
    store.hiredLawyerTier = tier;
    store.hasRetainer = true;
    store.activeCaseId += 1;
    return true;
}

int32_t rollCourtAcquittalBonusPercent(const PlayerLegalCounselStore& store, uint64_t worldSeed, uint64_t tickCount) {
    const LawyerTierDefinition* lawyer = getLawyerTierDefinition(store.hiredLawyerTier);
    const uint32_t roll = static_cast<uint32_t>((worldSeed ^ tickCount) % 20ULL);
    return lawyer->acquittalBonusPercent + static_cast<int32_t>(roll);
}

int32_t rollPrisonSentenceReductionPercent(const PlayerLegalCounselStore& store) {
    const LawyerTierDefinition* lawyer = getLawyerTierDefinition(store.hiredLawyerTier);
    return lawyer->sentenceReductionPercent;
}

} // namespace Core
