#pragma once

#include "game/economy_constants.h"
#include <cstddef>
#include <cstdint>

namespace Core {

struct PlayerWallet {
    int64_t cashCents = 0;
    int64_t lifetimeLegitCents = 0;
    int64_t lifetimeCrimeCents = 0;
    float legitIncomePerTickCents = 0.0f;
    float crimeIncomePerTickCents = 0.0f;
    int64_t lastDeltaCents = 0;
    WalletDeltaKind lastDeltaKind = WalletDeltaKind::None;
    float accruedLegitCents = 0.0f;
    float accruedCrimeCents = 0.0f;
};

void formatCashCents(char* outBuffer, size_t bufferSize, int64_t cashCents);
int64_t rollStartingCashCents(uint64_t seed, int32_t salt);
bool canAffordCash(const PlayerWallet& wallet, int64_t costCents);
bool tryDebitCash(PlayerWallet& wallet, int64_t costCents);
void creditLegitCash(PlayerWallet& wallet, int64_t amountCents);
void creditCrimeCash(PlayerWallet& wallet, int64_t amountCents);
void clearWalletDeltaDisplay(PlayerWallet& wallet);
void restoreCashWithoutDelta(PlayerWallet& wallet, int64_t amountCents);
bool isWalletBroke(const PlayerWallet& wallet);

} // namespace Core
