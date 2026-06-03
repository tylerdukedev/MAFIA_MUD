#include "game/player_wallet.h"
#include <algorithm>
#include <catch2/catch_test_macros.hpp>

using namespace Core;

TEST_CASE("PlayerWallet starting cash roll stays in weighted range", "[economy]") {
    int64_t zeroCount = 0;
    int64_t maxCents = 0;
    constexpr int32_t ROLL_SAMPLES = 200;
    for (int32_t sample = 0; sample < ROLL_SAMPLES; ++sample) {
        const int64_t cents = rollStartingCashCents(static_cast<uint64_t>(sample + 1), 0);
        if (cents == 0) {
            ++zeroCount;
        }
        maxCents = std::max(maxCents, cents);
        REQUIRE(cents >= 0);
        REQUIRE(cents <= STARTING_CASH_MAX_DOLLARS * CENTS_PER_DOLLAR);
    }
    REQUIRE(zeroCount > 0);
    REQUIRE(maxCents == STARTING_CASH_MAX_DOLLARS * CENTS_PER_DOLLAR);
}

TEST_CASE("PlayerWallet debit and credit update balances", "[economy]") {
    PlayerWallet wallet{};
    wallet.cashCents = 1000;
    REQUIRE(tryDebitCash(wallet, 400));
    REQUIRE(wallet.cashCents == 600);
    REQUIRE(wallet.lastDeltaKind == WalletDeltaKind::Loss);
    creditLegitCash(wallet, 250);
    REQUIRE(wallet.cashCents == 850);
    REQUIRE(wallet.lifetimeLegitCents == 250);
    creditCrimeCash(wallet, 100);
    REQUIRE(wallet.cashCents == 950);
    REQUIRE(wallet.lifetimeCrimeCents == 100);
    REQUIRE(isWalletBroke(wallet) == false);
    wallet.cashCents = BROKE_CASH_THRESHOLD_CENTS;
    REQUIRE(isWalletBroke(wallet));
}
