#pragma once

#include "game/bank_loan.h"
#include "game/game_calendar.h"
#include "game/player_operations.h"
#include "game/player_wallet.h"
#include "game/property_listing_store.h"
#include "game/property_store.h"
#include "world/tile_vitality.h"
#include <cstdint>

namespace Core {

constexpr int32_t REAL_ESTATE_COMMISSION_BPS = 600;
constexpr int32_t MORTGAGE_DOWN_PAYMENT_BPS = 2000;
constexpr int32_t MORTGAGE_TERM_MONTHS = 360;

const char* propertyListingTierToLabel(PropertyListingTier tier);
bool tryRentPropertyListing(
    int32_t listingIndex,
    PropertyListingStore& listingStore,
    PropertyStore& propertyStore,
    PlayerOperationsStore& operationsStore,
    PlayerWallet& wallet,
    const PlayerProfile& profile,
    uint64_t tickCount);
bool tryPurchasePropertyListing(
    int32_t listingIndex,
    PropertyListingStore& listingStore,
    PropertyStore& propertyStore,
    BankLoanStore& loanStore,
    PlayerOperationsStore& operationsStore,
    PlayerWallet& wallet,
    const PlayerProfile& profile,
    const GameCalendarStore& calendarStore,
    const BoroughVitalityStore& boroughVitalityStore,
    uint64_t tickCount,
    bool useMortgage);
void renderPropertyBrowserSection(
    PropertyListingStore& listingStore,
    PropertyStore& propertyStore,
    BankLoanStore& loanStore,
    PlayerOperationsStore& operationsStore,
    PlayerWallet& wallet,
    const PlayerProfile& profile,
    const GameCalendarStore& calendarStore,
    const BoroughVitalityStore& boroughVitalityStore,
    uint64_t tickCount);

} // namespace Core
