#pragma once

#include "game/operation_types.h"
#include "game/player_operations.h"
#include "game/player_wallet.h"
#include "sim/character_agent.h"
#include <cstdint>

namespace Core {

constexpr int32_t MONTHLY_LEDGER_INTERVAL_TICKS = 800;
constexpr int32_t FAMILY_UPKEEP_COOLDOWN_TICKS = 160;
constexpr int32_t MAX_FAMILY_UPKEEP_ACTION_COUNT = 8;
constexpr int32_t FAMILY_DPA_MONTHLY_OPINION_DRAIN = 1;

struct HousingBillLine {
    const char* label;
    int64_t amountCents;
};

struct MonthlyHousingLedger {
    int64_t rentCents = 0;
    int64_t utilitiesCents = 0;
    int64_t taxesAndFeesCents = 0;
    int64_t jobIncomeCents = 0;
    int64_t totalExpenseCents = 0;
    int64_t netCashDeltaCents = 0;
};

struct FamilyUpkeepActionDefinition {
    const char* displayName;
    int64_t costCents;
    int32_t opinionBonus;
    int32_t trustBonus;
    int32_t respectBonus;
    int32_t targetAgentIndex;
};

int32_t computeEffectiveRentMultiplierBps(const PlayerOperationsStore& store);
void buildMonthlyHousingLedger(
    const PlayerOperationsStore& store,
    int32_t employedBusinessIndex,
    MonthlyHousingLedger& outLedger);
bool shouldRunMonthlyLedger(const PlayerOperationsStore& store, uint64_t tickCount);
void applyMonthlyLivingLedger(
    PlayerOperationsStore& store,
    PlayerWallet& wallet,
    CharacterAgentStore& agentStore,
    int32_t employedBusinessIndex,
    uint64_t tickCount);
int32_t getFamilyUpkeepActionCount();
const FamilyUpkeepActionDefinition* getFamilyUpkeepActionDefinition(int32_t actionIndex);
bool canApplyFamilyUpkeep(const PlayerOperationsStore& store, uint64_t tickCount);
bool tryApplyFamilyUpkeep(
    PlayerOperationsStore& store,
    PlayerWallet& wallet,
    CharacterAgentStore& agentStore,
    int32_t actionIndex,
    uint64_t tickCount);

} // namespace Core
