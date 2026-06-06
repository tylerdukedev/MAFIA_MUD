#include "ui/real_estate_ui.h"
#include "character/player_profile.h"
#include "game/player_wallet.h"
#include "world/landmark_table.h"
#include "world/region_table.h"
#include "imgui.h"
#include <cstdio>

namespace Core {

const char* propertyListingTierToLabel(PropertyListingTier tier) {
    switch (tier) {
        case PropertyListingTier::Room: return "Room";
        case PropertyListingTier::Apartment: return "Apartment";
        case PropertyListingTier::House: return "House";
        case PropertyListingTier::MultiFamily: return "Multi-family";
        default: return "Listing";
    }
}

namespace {

HeadquartersKind headquartersKindFromListingTier(PropertyListingTier tier) {
    if (tier == PropertyListingTier::Room) {
        return HeadquartersKind::RentedRoom;
    }
    return HeadquartersKind::Apartment;
}

int64_t computeCommissionCents(int64_t baseCents) {
    return (baseCents * static_cast<int64_t>(REAL_ESTATE_COMMISSION_BPS)) / 10000LL;
}

bool listingMatchesPlayerBorough(const PropertyListingRecord& listing, const PlayerProfile& profile) {
    const RegionId playerRegion = regionIdFromBoroughPreferenceIndex(profile.draft.selectedBoroughIndex);
    return listing.regionId == static_cast<uint8_t>(playerRegion);
}

} // namespace

bool tryRentPropertyListing(
    int32_t listingIndex,
    PropertyListingStore& listingStore,
    PropertyStore& propertyStore,
    PlayerOperationsStore& operationsStore,
    PlayerWallet& wallet,
    const PlayerProfile& profile,
    uint64_t tickCount) {
    if (hasPlayerHeadquarters(operationsStore)) {
        return false;
    }
    if (listingIndex < 0 || listingIndex >= listingStore.listingCount) {
        return false;
    }
    PropertyListingRecord& listing = listingStore.records[listingIndex];
    if (!listing.onMarket) {
        return false;
    }
    if (!listingMatchesPlayerBorough(listing, profile)) {
        return false;
    }
    const int64_t commissionCents = computeCommissionCents(listing.rentCents);
    const int64_t moveInCostCents = listing.rentCents + commissionCents;
    if (wallet.cashCents < moveInCostCents) {
        return false;
    }
    if (listing.propertyIndex < 0) {
        return false;
    }
    if (!tryAssignPropertyToPlayer(propertyStore, listing.propertyIndex)) {
        return false;
    }
    wallet.cashCents -= moveInCostCents;
    operationsStore.headquartersKind = headquartersKindFromListingTier(listing.tier);
    operationsStore.homePropertyIndex = listingIndex;
    operationsStore.housingTenure = HousingTenure::Rent;
    operationsStore.headquartersEstablishedTick = tickCount;
    operationsStore.headquartersRegionId = listing.regionId;
    operationsStore.lastMonthlyLedgerTick = tickCount;
    listing.onMarket = false;
    return true;
}

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
    bool useMortgage) {
    if (hasPlayerHeadquarters(operationsStore)) {
        return false;
    }
    if (listingIndex < 0 || listingIndex >= listingStore.listingCount) {
        return false;
    }
    PropertyListingRecord& listing = listingStore.records[listingIndex];
    if (!listing.onMarket) {
        return false;
    }
    if (!listingMatchesPlayerBorough(listing, profile)) {
        return false;
    }
    if (listing.propertyIndex < 0) {
        return false;
    }
    const int64_t commissionCents = computeCommissionCents(listing.askPriceCents);
    int64_t dueAtClosingCents = listing.askPriceCents + commissionCents;
    if (useMortgage) {
        const int64_t downPaymentCents = (listing.askPriceCents * static_cast<int64_t>(MORTGAGE_DOWN_PAYMENT_BPS)) / 10000LL;
        dueAtClosingCents = downPaymentCents + commissionCents;
        if (wallet.cashCents < dueAtClosingCents) {
            return false;
        }
        const int64_t principalCents = listing.askPriceCents - downPaymentCents;
        const int32_t loanIndex = tryOpenLoan(
            loanStore,
            LoanKind::Mortgage,
            principalCents,
            MORTGAGE_TERM_MONTHS,
            static_cast<RegionId>(listing.regionId),
            calendarStore,
            boroughVitalityStore);
        if (loanIndex < 0) {
            return false;
        }
    } else if (wallet.cashCents < dueAtClosingCents) {
        return false;
    }
    if (!tryAssignPropertyToPlayer(propertyStore, listing.propertyIndex)) {
        return false;
    }
    wallet.cashCents -= dueAtClosingCents;
    operationsStore.headquartersKind = headquartersKindFromListingTier(listing.tier);
    operationsStore.homePropertyIndex = listingIndex;
    operationsStore.housingTenure = HousingTenure::Own;
    operationsStore.headquartersEstablishedTick = tickCount;
    operationsStore.headquartersRegionId = listing.regionId;
    operationsStore.lastMonthlyLedgerTick = tickCount;
    listing.onMarket = false;
    return true;
}

void renderPropertyBrowserSection(
    PropertyListingStore& listingStore,
    PropertyStore& propertyStore,
    BankLoanStore& loanStore,
    PlayerOperationsStore& operationsStore,
    PlayerWallet& wallet,
    const PlayerProfile& profile,
    const GameCalendarStore& calendarStore,
    const BoroughVitalityStore& boroughVitalityStore,
    uint64_t tickCount) {
    if (hasPlayerHeadquarters(operationsStore)) {
        return;
    }
    ImGui::Separator();
    ImGui::Text("Real estate listings (your borough)");
    ImGui::TextWrapped("Visit a real estate office on the map, or rent/buy below.");
    bool hasAnyListing = false;
    for (int32_t listingIndex = 0; listingIndex < listingStore.listingCount; ++listingIndex) {
        const PropertyListingRecord* listing = getPropertyListingRecord(listingStore, listingIndex);
        if (listing == nullptr || !listing->onMarket) {
            continue;
        }
        if (!listingMatchesPlayerBorough(*listing, profile)) {
            continue;
        }
        hasAnyListing = true;
        char rentBuffer[32];
        char priceBuffer[32];
        formatCashCents(rentBuffer, sizeof(rentBuffer), listing->rentCents);
        formatCashCents(priceBuffer, sizeof(priceBuffer), listing->askPriceCents);
        ImGui::PushID(listingIndex);
        ImGui::BulletText(
            "%s — rent %s/mo | buy %s | %d bed",
            propertyListingTierToLabel(listing->tier),
            rentBuffer,
            priceBuffer,
            static_cast<int32_t>(listing->beds));
        if (ImGui::Button("Rent")) {
            tryRentPropertyListing(
                listingIndex, listingStore, propertyStore, operationsStore, wallet, profile, tickCount);
        }
        ImGui::SameLine();
        if (ImGui::Button("Buy (cash)")) {
            tryPurchasePropertyListing(
                listingIndex, listingStore, propertyStore, loanStore, operationsStore, wallet, profile,
                calendarStore, boroughVitalityStore, tickCount, false);
        }
        ImGui::SameLine();
        if (ImGui::Button("Buy (mortgage)")) {
            tryPurchasePropertyListing(
                listingIndex, listingStore, propertyStore, loanStore, operationsStore, wallet, profile,
                calendarStore, boroughVitalityStore, tickCount, true);
        }
        ImGui::PopID();
    }
    if (!hasAnyListing) {
        ImGui::TextDisabled("No on-market listings in your borough yet.");
    }
}

} // namespace Core
