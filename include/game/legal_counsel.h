#pragma once

#include "game/player_criminal_justice.h"
#include "game/player_wallet.h"
#include <cstdint>

namespace Core {

enum class LawyerTier : uint8_t {
    PublicDefender = 0,
    Neighborhood = 1,
    Experienced = 2,
    HighProfile = 3,
};

struct LawyerTierDefinition {
    LawyerTier tier;
    const char* displayName;
    int64_t retainerCents;
    int32_t sentenceReductionPercent;
    int32_t acquittalBonusPercent;
};

struct PlayerLegalCounselStore {
    LawyerTier hiredLawyerTier = LawyerTier::PublicDefender;
    bool hasRetainer = false;
    int32_t activeCaseId = 0;
};

void resetPlayerLegalCounselStore(PlayerLegalCounselStore& store);
const LawyerTierDefinition* getLawyerTierDefinition(LawyerTier tier);
int32_t getLawyerTierCount();
bool tryHireLawyer(PlayerLegalCounselStore& store, PlayerWallet& wallet, LawyerTier tier);
int32_t rollCourtAcquittalBonusPercent(const PlayerLegalCounselStore& store, uint64_t worldSeed, uint64_t tickCount);
int32_t rollPrisonSentenceReductionPercent(const PlayerLegalCounselStore& store);

} // namespace Core
