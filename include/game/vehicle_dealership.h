#pragma once

#include "game/bank_loan.h"
#include "game/game_calendar.h"
#include "game/player_wallet.h"
#include "world/tile_vitality.h"
#include <cstdint>

namespace Core {

constexpr int32_t MAX_DEALERSHIP_INVENTORY = 6;

enum class VehiclePurchaseMethod : uint8_t {
    Cash = 0,
    Loan = 1,
};

struct VehicleListing {
    const char* make;
    const char* model;
    int32_t introYear;
    int64_t basePriceCents;
};

int32_t getDealershipInventoryCount(int32_t businessIndex);
const VehicleListing* getDealershipVehicleListing(int32_t businessIndex, int32_t inventoryIndex);
int64_t computeVehicleListPriceCents(
    int32_t businessIndex,
    int32_t inventoryIndex,
    const GameCalendarStore& calendarStore,
    const BoroughVitalityStore& boroughVitalityStore);
bool tryPurchaseVehicle(
    int32_t businessIndex,
    int32_t inventoryIndex,
    VehiclePurchaseMethod method,
    PlayerWallet& wallet,
    BankLoanStore& loanStore,
    const GameCalendarStore& calendarStore,
    const BoroughVitalityStore& boroughVitalityStore);

} // namespace Core
