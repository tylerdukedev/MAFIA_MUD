#include "game/vehicle_dealership.h"
#include "game/economy_index.h"
#include "world/business_node_table.h"
#include <algorithm>

namespace Core {

namespace {

// Prices in cents, scaled to the in-game economy (a room buys for ~$1,500,
// a house ~$5,500) and to era-accurate sticker prices: a 1928 Model A sold
// new for ~$550, a used Model T for ~$260.
constexpr VehicleListing NEW_VEHICLE_INVENTORY[] = {
    {"Ford", "Model A Tudor", 1928, 60000},
    {"Chevrolet", "International", 1929, 70000},
    {"Buick", "Series 40", 1930, 110000},
    {"Plymouth", "Model 30U", 1931, 58000},
    {"Dodge", "DC Series", 1932, 68000},
    {"Chrysler", "Royal Eight", 1933, 145000},
};

constexpr VehicleListing USED_VEHICLE_INVENTORY[] = {
    {"Ford", "Model T Runabout", 1925, 22000},
    {"Chevrolet", "Superior", 1926, 26000},
    {"Hudson", "Essex Coach", 1927, 30000},
    {"Nash", "Special Six", 1928, 34000},
    {"Studebaker", "Dictator", 1929, 38000},
    {"DeSoto", "SA Coupe", 1930, 42000},
};

constexpr int32_t NEW_INVENTORY_COUNT = static_cast<int32_t>(sizeof(NEW_VEHICLE_INVENTORY) / sizeof(NEW_VEHICLE_INVENTORY[0]));
constexpr int32_t USED_INVENTORY_COUNT = static_cast<int32_t>(sizeof(USED_VEHICLE_INVENTORY) / sizeof(USED_VEHICLE_INVENTORY[0]));

constexpr int32_t ERA_DISCOUNT_BPS_PER_YEAR = 350;
constexpr int32_t ERA_PREMIUM_BPS_PER_YEAR = 250;
constexpr int32_t USED_VEHICLE_DISCOUNT_BPS = 8200;
constexpr int32_t AUTO_LOAN_TERM_MONTHS = 36;
constexpr int32_t AUTO_LOAN_DOWN_PAYMENT_BPS = 2000;

const VehicleListing* resolveInventory(int32_t businessIndex, int32_t& outCount) {
    const BusinessNodeDefinition* business = getBusinessNodeDefinition(businessIndex);
    if (business == nullptr) {
        outCount = 0;
        return nullptr;
    }
    if (business->kind == BusinessNodeKind::CarDealershipNew) {
        outCount = NEW_INVENTORY_COUNT;
        return NEW_VEHICLE_INVENTORY;
    }
    if (business->kind == BusinessNodeKind::CarDealershipUsed) {
        outCount = USED_INVENTORY_COUNT;
        return USED_VEHICLE_INVENTORY;
    }
    outCount = 0;
    return nullptr;
}

int32_t computeEraPriceAdjustmentBps(int32_t introYear, int32_t currentYear) {
    const int32_t yearDelta = currentYear - introYear;
    if (yearDelta >= 0) {
        return std::max(7000, ECONOMY_MULTIPLIER_BPS_BASE - yearDelta * ERA_DISCOUNT_BPS_PER_YEAR);
    }
    const int32_t futureYears = -yearDelta;
    return ECONOMY_MULTIPLIER_BPS_BASE + futureYears * ERA_PREMIUM_BPS_PER_YEAR;
}

} // namespace

int32_t getDealershipInventoryCount(int32_t businessIndex) {
    int32_t inventoryCount = 0;
    resolveInventory(businessIndex, inventoryCount);
    return std::min(inventoryCount, MAX_DEALERSHIP_INVENTORY);
}

const VehicleListing* getDealershipVehicleListing(int32_t businessIndex, int32_t inventoryIndex) {
    int32_t inventoryCount = 0;
    const VehicleListing* inventory = resolveInventory(businessIndex, inventoryCount);
    if (inventory == nullptr || inventoryIndex < 0 || inventoryIndex >= inventoryCount) {
        return nullptr;
    }
    return &inventory[inventoryIndex];
}

int64_t computeVehicleListPriceCents(
    int32_t businessIndex,
    int32_t inventoryIndex,
    const GameCalendarStore& calendarStore,
    const BoroughVitalityStore& boroughVitalityStore) {
    const VehicleListing* listing = getDealershipVehicleListing(businessIndex, inventoryIndex);
    if (listing == nullptr) {
        return 0;
    }
    const BusinessNodeDefinition* business = getBusinessNodeDefinition(businessIndex);
    const RegionId regionId = getBusinessNodeRegionId(businessIndex);
    const int32_t economyBps = getCombinedEconomyMultiplierBps(regionId, calendarStore, boroughVitalityStore);
    const int32_t eraBps = computeEraPriceAdjustmentBps(listing->introYear, calendarStore.year);
    int32_t combinedBps = (economyBps * eraBps) / ECONOMY_MULTIPLIER_BPS_BASE;
    if (business != nullptr && business->kind == BusinessNodeKind::CarDealershipUsed) {
        combinedBps = (combinedBps * USED_VEHICLE_DISCOUNT_BPS) / ECONOMY_MULTIPLIER_BPS_BASE;
    }
    return scalePriceCentsByEconomy(listing->basePriceCents, combinedBps);
}

bool tryPurchaseVehicle(
    int32_t businessIndex,
    int32_t inventoryIndex,
    VehiclePurchaseMethod method,
    PlayerWallet& wallet,
    BankLoanStore& loanStore,
    const GameCalendarStore& calendarStore,
    const BoroughVitalityStore& boroughVitalityStore) {
    if (!isCarDealershipBusinessIndex(businessIndex)) {
        return false;
    }
    const int64_t listPriceCents = computeVehicleListPriceCents(
        businessIndex,
        inventoryIndex,
        calendarStore,
        boroughVitalityStore);
    if (listPriceCents <= 0) {
        return false;
    }
    const RegionId regionId = getBusinessNodeRegionId(businessIndex);
    if (method == VehiclePurchaseMethod::Cash) {
        return tryDebitCash(wallet, listPriceCents);
    }
    const int64_t downPaymentCents = (listPriceCents * static_cast<int64_t>(AUTO_LOAN_DOWN_PAYMENT_BPS))
        / static_cast<int64_t>(ECONOMY_MULTIPLIER_BPS_BASE);
    const int64_t financedCents = listPriceCents - downPaymentCents;
    if (financedCents <= 0 || !canAffordCash(wallet, downPaymentCents)) {
        return false;
    }
    if (!tryDebitCash(wallet, downPaymentCents)) {
        return false;
    }
    const int32_t loanIndex = tryOpenLoan(
        loanStore,
        LoanKind::Auto,
        financedCents,
        AUTO_LOAN_TERM_MONTHS,
        regionId,
        calendarStore,
        boroughVitalityStore);
    if (loanIndex < 0) {
        restoreCashWithoutDelta(wallet, downPaymentCents);
        return false;
    }
    return true;
}

} // namespace Core
