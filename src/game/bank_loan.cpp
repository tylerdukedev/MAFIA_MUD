#include "game/bank_loan.h"
#include "game/economy_index.h"
#include "game/housing_living_costs.h"
#include <algorithm>

namespace Core {

namespace {

constexpr int32_t MORTGAGE_BASE_APR_BPS = 500;
constexpr int32_t AUTO_BASE_APR_BPS = 800;
constexpr int32_t LOAN_APR_ECONOMY_WEIGHT_BPS = 1200;
constexpr int32_t DEFAULT_MORTGAGE_TERM_MONTHS = 240;
constexpr int32_t DEFAULT_AUTO_TERM_MONTHS = 48;

int32_t findInactiveLoanSlot(const BankLoanStore& store) {
    for (int32_t index = 0; index < MAX_ACTIVE_LOANS; ++index) {
        if (!store.loans[index].isActive) {
            return index;
        }
    }
    return -1;
}

void debitLoanPaymentBestEffort(PlayerWallet& wallet, int64_t paymentCents) {
    if (paymentCents <= 0) {
        return;
    }
    if (tryDebitCash(wallet, paymentCents)) {
        return;
    }
    if (wallet.cashCents <= 0) {
        return;
    }
    const int64_t available = wallet.cashCents;
    wallet.cashCents = 0;
    wallet.lastDeltaCents = available;
    wallet.lastDeltaKind = WalletDeltaKind::Loss;
}

} // namespace

void resetBankLoanStore(BankLoanStore& store) {
    for (int32_t index = 0; index < MAX_ACTIVE_LOANS; ++index) {
        store.loans[index] = BankLoanRecord{};
    }
    store.activeLoanCount = 0;
    store.lastMonthlyPaymentTick = 0;
}

int32_t computeLoanAprBps(
    LoanKind kind,
    RegionId regionId,
    const GameCalendarStore& calendarStore,
    const BoroughVitalityStore& boroughVitalityStore) {
    const int32_t baseAprBps = kind == LoanKind::Mortgage ? MORTGAGE_BASE_APR_BPS : AUTO_BASE_APR_BPS;
    const int32_t economyBps = getCombinedEconomyMultiplierBps(regionId, calendarStore, boroughVitalityStore);
    const int32_t economyDeltaBps = economyBps - ECONOMY_MULTIPLIER_BPS_BASE;
    const int32_t adjustedAprBps = baseAprBps + (economyDeltaBps * LOAN_APR_ECONOMY_WEIGHT_BPS) / ECONOMY_MULTIPLIER_BPS_BASE;
    return std::clamp(adjustedAprBps, 350, 1800);
}

int64_t computeLoanMonthlyPaymentCents(int64_t principalCents, int32_t aprBps, int32_t termMonths) {
    if (principalCents <= 0 || termMonths <= 0) {
        return 0;
    }
    const int64_t totalInterestCents = (principalCents * static_cast<int64_t>(aprBps) * static_cast<int64_t>(termMonths))
        / (10000LL * 12LL);
    return (principalCents + totalInterestCents) / static_cast<int64_t>(termMonths);
}

int32_t tryOpenLoan(
    BankLoanStore& store,
    LoanKind kind,
    int64_t principalCents,
    int32_t termMonths,
    RegionId regionId,
    const GameCalendarStore& calendarStore,
    const BoroughVitalityStore& boroughVitalityStore) {
    if (principalCents <= 0) {
        return -1;
    }
    const int32_t slotIndex = findInactiveLoanSlot(store);
    if (slotIndex < 0) {
        return -1;
    }
    const int32_t resolvedTermMonths = termMonths > 0
        ? termMonths
        : (kind == LoanKind::Mortgage ? DEFAULT_MORTGAGE_TERM_MONTHS : DEFAULT_AUTO_TERM_MONTHS);
    const int32_t aprBps = computeLoanAprBps(kind, regionId, calendarStore, boroughVitalityStore);
    BankLoanRecord& loan = store.loans[slotIndex];
    loan.kind = kind;
    loan.principalRemainingCents = principalCents;
    loan.aprBps = aprBps;
    loan.termMonthsRemaining = resolvedTermMonths;
    loan.monthlyPaymentCents = computeLoanMonthlyPaymentCents(principalCents, aprBps, resolvedTermMonths);
    loan.isActive = true;
    loan.regionId = static_cast<uint8_t>(regionId);
    store.activeLoanCount += 1;
    return slotIndex;
}

bool tickMonthlyLoanPayments(BankLoanStore& store, PlayerWallet& wallet, uint64_t tickCount) {
    if (store.activeLoanCount <= 0) {
        return false;
    }
    if (store.lastMonthlyPaymentTick != 0ULL
        && tickCount - store.lastMonthlyPaymentTick < static_cast<uint64_t>(MONTHLY_LEDGER_INTERVAL_TICKS)) {
        return false;
    }
    store.lastMonthlyPaymentTick = tickCount;
    bool didProcessPayment = false;
    for (int32_t index = 0; index < MAX_ACTIVE_LOANS; ++index) {
        BankLoanRecord& loan = store.loans[index];
        if (!loan.isActive) {
            continue;
        }
        debitLoanPaymentBestEffort(wallet, loan.monthlyPaymentCents);
        loan.principalRemainingCents = std::max<int64_t>(0, loan.principalRemainingCents - loan.monthlyPaymentCents);
        loan.termMonthsRemaining = std::max(0, loan.termMonthsRemaining - 1);
        if (loan.principalRemainingCents <= 0 || loan.termMonthsRemaining <= 0) {
            loan.isActive = false;
            store.activeLoanCount = std::max(0, store.activeLoanCount - 1);
        }
        didProcessPayment = true;
    }
    return didProcessPayment;
}

} // namespace Core
