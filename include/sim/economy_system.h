#pragma once

#include "sim/isim_system.h"
#include "sim/sim_world_bindings.h"

namespace Core {

struct BankLoanStore;
struct GameCalendarStore;
struct PropertyListingStore;

class EconomySystem final : public ISimSystem {
public:
    void bind(
        const SimWorldBindings& bindings,
        BankLoanStore* loanStore = nullptr,
        GameCalendarStore* calendarStore = nullptr,
        PropertyListingStore* listingStore = nullptr);
    const char* getName() const override;
    void onTick(uint64_t tickCount) override;

private:
    SimWorldBindings bindings{};
    bool hasAppliedStartingInfluence = false;
    void recomputeIncomeRates();
    void applyAccruedIncome();
    void applyStartingBoroughInfluence();
    void applyMonthlyLivingCosts(uint64_t tickCount);
    void applyMonthlyLoanPayments(uint64_t tickCount);

    BankLoanStore* loanStore = nullptr;
    GameCalendarStore* calendarStore = nullptr;
    PropertyListingStore* propertyListingStore = nullptr;
};

} // namespace Core
