#pragma once

#include "core/types.h"
#include "game/game_calendar.h"
#include "game/player_wallet.h"
#include "world/tile_vitality.h"
#include <cstdint>

namespace Core {

constexpr int32_t MAX_ACTIVE_LOANS = 16;

enum class LoanKind : uint8_t {
    Mortgage = 0,
    Auto = 1,
};

struct BankLoanRecord {
    LoanKind kind = LoanKind::Mortgage;
    int64_t principalRemainingCents = 0;
    int32_t aprBps = 0;
    int32_t termMonthsRemaining = 0;
    int64_t monthlyPaymentCents = 0;
    bool isActive = false;
    uint8_t regionId = 0;
};

struct BankLoanStore {
    BankLoanRecord loans[MAX_ACTIVE_LOANS]{};
    int32_t activeLoanCount = 0;
    uint64_t lastMonthlyPaymentTick = 0;
};

void resetBankLoanStore(BankLoanStore& store);
int32_t computeLoanAprBps(
    LoanKind kind,
    RegionId regionId,
    const GameCalendarStore& calendarStore,
    const BoroughVitalityStore& boroughVitalityStore);
int64_t computeLoanMonthlyPaymentCents(int64_t principalCents, int32_t aprBps, int32_t termMonths);
int32_t tryOpenLoan(
    BankLoanStore& store,
    LoanKind kind,
    int64_t principalCents,
    int32_t termMonths,
    RegionId regionId,
    const GameCalendarStore& calendarStore,
    const BoroughVitalityStore& boroughVitalityStore);
bool tickMonthlyLoanPayments(
    BankLoanStore& store,
    PlayerWallet& wallet,
    uint64_t tickCount);

} // namespace Core
