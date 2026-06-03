#include "game/player_wallet.h"
#include "utils/seed_hash.h"
#include <cstdio>

namespace Core {

namespace {
constexpr int32_t STARTING_CASH_WEIGHT_AT_ZERO = 100;
constexpr int32_t STARTING_CASH_WEIGHT_AT_MAX = 20;
constexpr int32_t STARTING_CASH_DOLLAR_SPAN = 25;
} // namespace

void formatCashCents(char* outBuffer, size_t bufferSize, int64_t cashCents) {
    if (outBuffer == nullptr || bufferSize == 0) {
        return;
    }
    const int64_t absCents = cashCents < 0 ? -cashCents : cashCents;
    const int64_t dollars = absCents / CENTS_PER_DOLLAR;
    const int64_t cents = absCents % CENTS_PER_DOLLAR;
    if (cashCents < 0) {
        std::snprintf(outBuffer, bufferSize, "-$%lld.%02lld", static_cast<long long>(dollars), static_cast<long long>(cents));
        return;
    }
    std::snprintf(outBuffer, bufferSize, "$%lld.%02lld", static_cast<long long>(dollars), static_cast<long long>(cents));
}

int64_t rollStartingCashCents(uint64_t seed, int32_t salt) {
    int32_t totalWeight = 0;
    int32_t weights[static_cast<size_t>(STARTING_CASH_MAX_DOLLARS) + 1] = {};
    for (int32_t dollars = 0; dollars <= STARTING_CASH_DOLLAR_SPAN; ++dollars) {
        const int32_t weight = STARTING_CASH_WEIGHT_AT_ZERO - (STARTING_CASH_WEIGHT_AT_ZERO - STARTING_CASH_WEIGHT_AT_MAX) * dollars / STARTING_CASH_DOLLAR_SPAN;
        weights[static_cast<size_t>(dollars)] = weight;
        totalWeight += weight;
    }
    if (totalWeight <= 0) {
        return 0;
    }
    const uint32_t roll = Utils::hashSeedMix(seed, salt, 0x57414C4C) % static_cast<uint32_t>(totalWeight);
    int32_t cumulative = 0;
    for (int32_t dollars = 0; dollars <= STARTING_CASH_DOLLAR_SPAN; ++dollars) {
        cumulative += weights[static_cast<size_t>(dollars)];
        if (roll < static_cast<uint32_t>(cumulative)) {
            return static_cast<int64_t>(dollars) * CENTS_PER_DOLLAR;
        }
    }
    return 0;
}

bool canAffordCash(const PlayerWallet& wallet, int64_t costCents) {
    return wallet.cashCents >= costCents;
}

bool tryDebitCash(PlayerWallet& wallet, int64_t costCents) {
    if (!canAffordCash(wallet, costCents)) {
        return false;
    }
    wallet.cashCents -= costCents;
    wallet.lastDeltaCents = -costCents;
    wallet.lastDeltaKind = WalletDeltaKind::Loss;
    return true;
}

void creditLegitCash(PlayerWallet& wallet, int64_t amountCents) {
    if (amountCents <= 0) {
        return;
    }
    wallet.cashCents += amountCents;
    wallet.lifetimeLegitCents += amountCents;
    wallet.lastDeltaCents = amountCents;
    wallet.lastDeltaKind = WalletDeltaKind::GainLegit;
}

void creditCrimeCash(PlayerWallet& wallet, int64_t amountCents) {
    if (amountCents <= 0) {
        return;
    }
    wallet.cashCents += amountCents;
    wallet.lifetimeCrimeCents += amountCents;
    wallet.lastDeltaCents = amountCents;
    wallet.lastDeltaKind = WalletDeltaKind::GainCrime;
}

void clearWalletDeltaDisplay(PlayerWallet& wallet) {
    wallet.lastDeltaCents = 0;
    wallet.lastDeltaKind = WalletDeltaKind::None;
}

void restoreCashWithoutDelta(PlayerWallet& wallet, int64_t amountCents) {
    wallet.cashCents += amountCents;
}

bool isWalletBroke(const PlayerWallet& wallet) {
    return wallet.cashCents <= BROKE_CASH_THRESHOLD_CENTS;
}

} // namespace Core
