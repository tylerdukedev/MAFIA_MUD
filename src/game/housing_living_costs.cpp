#include "game/housing_living_costs.h"
#include "character/character_social_network.h"
#include "sim/world_event_types.h"
#include "world/business_node_table.h"
#include <algorithm>

namespace Core {

namespace {

constexpr int64_t RENTED_ROOM_MONTHLY_RENT_CENTS = 1400;
constexpr int64_t RENTED_ROOM_MONTHLY_TAX_CENTS = 150;
constexpr int64_t RENTED_ROOM_MONTHLY_FEE_CENTS = 100;
constexpr int64_t APARTMENT_MONTHLY_RENT_CENTS = 2800;
constexpr int64_t APARTMENT_GAS_HEAT_CENTS = 450;
constexpr int64_t APARTMENT_WATER_CLOSET_CENTS = 175;
constexpr int64_t APARTMENT_SEWER_SCAVENGER_CENTS = 125;
constexpr int64_t APARTMENT_ELECTRIC_CENTS = 200;
constexpr int64_t APARTMENT_CITY_TAX_CENTS = 350;

constexpr FamilyUpkeepActionDefinition FAMILY_UPKEEP_ACTIONS[] = {
    {"Cook dinner", 0, 2, 1, 0, FAMILY_AGENT_SLOT_INDEX},
    {"Buy dinner", 175, 4, 2, 1, FAMILY_AGENT_SLOT_INDEX},
    {"Wash dishes / chores", 0, 1, 2, 1, FAMILY_AGENT_SLOT_INDEX},
    {"Run errands", 50, 2, 2, 0, FRIEND_AGENT_SLOT_INDEX},
    {"Small gift", 400, 5, 3, 2, FAMILY_AGENT_SLOT_INDEX},
    {"Contribute to groceries", 300, 3, 3, 2, FAMILY_AGENT_SLOT_INDEX},
    {"Pay shared coal bill", 250, 4, 2, 2, FRIEND_AGENT_SLOT_INDEX},
    {"Respectful silence (rest)", 0, 1, 1, 1, FRIEND_AGENT_SLOT_INDEX},
};

constexpr int32_t FAMILY_UPKEEP_ACTION_COUNT = static_cast<int32_t>(sizeof(FAMILY_UPKEEP_ACTIONS) / sizeof(FAMILY_UPKEEP_ACTIONS[0]));

int64_t scaleHousingCentsByRentMultiplier(int64_t baseCents, int32_t effectiveMultiplierBps) {
    if (baseCents <= 0) {
        return 0;
    }
    return (baseCents * static_cast<int64_t>(effectiveMultiplierBps)) / static_cast<int64_t>(RENT_MULTIPLIER_BPS_BASE);
}

void debitCashBestEffort(PlayerWallet& wallet, int64_t costCents) {
    if (costCents <= 0) {
        return;
    }
    if (tryDebitCash(wallet, costCents)) {
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

void applyFamilyDpaMonthlyDrain(CharacterAgentStore& agentStore) {
    if (agentStore.states[FAMILY_AGENT_SLOT_INDEX].isActive) {
        adjustAgentOpinion(agentStore, FAMILY_AGENT_SLOT_INDEX, -FAMILY_DPA_MONTHLY_OPINION_DRAIN);
    }
    if (agentStore.states[FRIEND_AGENT_SLOT_INDEX].isActive) {
        adjustAgentOpinion(agentStore, FRIEND_AGENT_SLOT_INDEX, -FAMILY_DPA_MONTHLY_OPINION_DRAIN);
    }
}

} // namespace

int32_t computeEffectiveRentMultiplierBps(const PlayerOperationsStore& store) {
    const int32_t combined = store.rentMultiplierBps + store.rentEventAdjustmentBps;
    return std::clamp(combined, 7500, 14000);
}

void buildMonthlyHousingLedger(
    const PlayerOperationsStore& store,
    int32_t employedBusinessIndex,
    MonthlyHousingLedger& outLedger) {
    outLedger = MonthlyHousingLedger{};
    if (!hasPlayerHeadquarters(store)) {
        return;
    }
    const int32_t rentMultiplierBps = computeEffectiveRentMultiplierBps(store);
    if (store.headquartersKind == HeadquartersKind::RentedRoom) {
        outLedger.rentCents = scaleHousingCentsByRentMultiplier(RENTED_ROOM_MONTHLY_RENT_CENTS, rentMultiplierBps);
        outLedger.taxesAndFeesCents = scaleHousingCentsByRentMultiplier(RENTED_ROOM_MONTHLY_TAX_CENTS + RENTED_ROOM_MONTHLY_FEE_CENTS, rentMultiplierBps);
    } else if (store.headquartersKind == HeadquartersKind::Apartment) {
        outLedger.rentCents = scaleHousingCentsByRentMultiplier(APARTMENT_MONTHLY_RENT_CENTS, rentMultiplierBps);
        outLedger.utilitiesCents = scaleHousingCentsByRentMultiplier(
            APARTMENT_GAS_HEAT_CENTS + APARTMENT_WATER_CLOSET_CENTS + APARTMENT_SEWER_SCAVENGER_CENTS + APARTMENT_ELECTRIC_CENTS,
            rentMultiplierBps);
        outLedger.taxesAndFeesCents = scaleHousingCentsByRentMultiplier(APARTMENT_CITY_TAX_CENTS, rentMultiplierBps);
    }
    if (employedBusinessIndex >= 0) {
        const BusinessNodeDefinition* business = getBusinessNodeDefinition(employedBusinessIndex);
        if (business != nullptr) {
            outLedger.jobIncomeCents = business->jobWageCents * JOB_MONTHLY_WAGE_MULTIPLIER;
        }
    }
    outLedger.totalExpenseCents = outLedger.rentCents + outLedger.utilitiesCents + outLedger.taxesAndFeesCents;
    outLedger.netCashDeltaCents = outLedger.jobIncomeCents - outLedger.totalExpenseCents;
}

bool shouldRunMonthlyLedger(const PlayerOperationsStore& store, uint64_t tickCount) {
    if (!hasPlayerHeadquarters(store)) {
        return false;
    }
    if (store.headquartersEstablishedTick == 0ULL) {
        return false;
    }
    if (tickCount < store.headquartersEstablishedTick + static_cast<uint64_t>(MONTHLY_LEDGER_INTERVAL_TICKS)) {
        return false;
    }
    if (store.lastMonthlyLedgerTick != 0ULL && tickCount - store.lastMonthlyLedgerTick < static_cast<uint64_t>(MONTHLY_LEDGER_INTERVAL_TICKS)) {
        return false;
    }
    return true;
}

void applyMonthlyLivingLedger(
    PlayerOperationsStore& store,
    PlayerWallet& wallet,
    CharacterAgentStore& agentStore,
    int32_t employedBusinessIndex,
    uint64_t tickCount) {
    if (!shouldRunMonthlyLedger(store, tickCount)) {
        return;
    }
    store.lastMonthlyLedgerTick = tickCount;
    MonthlyHousingLedger ledger{};
    buildMonthlyHousingLedger(store, employedBusinessIndex, ledger);
    if (ledger.jobIncomeCents > 0) {
        creditLegitCash(wallet, ledger.jobIncomeCents);
    }
    if (store.headquartersKind == HeadquartersKind::FamilyFriendDpa) {
        applyFamilyDpaMonthlyDrain(agentStore);
        return;
    }
    const int64_t totalOwed = ledger.rentCents + ledger.utilitiesCents + ledger.taxesAndFeesCents;
    const int64_t cashBeforePayment = wallet.cashCents;
    debitCashBestEffort(wallet, ledger.rentCents);
    debitCashBestEffort(wallet, ledger.utilitiesCents);
    debitCashBestEffort(wallet, ledger.taxesAndFeesCents);
    const int64_t cashPaid = cashBeforePayment - wallet.cashCents;
    if (cashPaid < totalOwed) {
        store.consecutiveUnpaidRentMonths = static_cast<int8_t>(std::min(12, static_cast<int32_t>(store.consecutiveUnpaidRentMonths) + 1));
        return;
    }
    store.consecutiveUnpaidRentMonths = 0;
}

int32_t getFamilyUpkeepActionCount() {
    return FAMILY_UPKEEP_ACTION_COUNT;
}

const FamilyUpkeepActionDefinition* getFamilyUpkeepActionDefinition(int32_t actionIndex) {
    if (actionIndex < 0 || actionIndex >= FAMILY_UPKEEP_ACTION_COUNT) {
        return nullptr;
    }
    return &FAMILY_UPKEEP_ACTIONS[actionIndex];
}

bool canApplyFamilyUpkeep(const PlayerOperationsStore& store, uint64_t tickCount) {
    if (store.headquartersKind != HeadquartersKind::FamilyFriendDpa) {
        return false;
    }
    if (store.lastFamilyUpkeepTick != 0ULL && tickCount - store.lastFamilyUpkeepTick < static_cast<uint64_t>(FAMILY_UPKEEP_COOLDOWN_TICKS)) {
        return false;
    }
    return true;
}

bool tryApplyFamilyUpkeep(
    PlayerOperationsStore& store,
    PlayerWallet& wallet,
    CharacterAgentStore& agentStore,
    int32_t actionIndex,
    uint64_t tickCount) {
    if (!canApplyFamilyUpkeep(store, tickCount)) {
        return false;
    }
    const FamilyUpkeepActionDefinition* action = getFamilyUpkeepActionDefinition(actionIndex);
    if (action == nullptr) {
        return false;
    }
    if (!agentStore.states[action->targetAgentIndex].isActive) {
        return false;
    }
    if (action->costCents > 0 && !tryDebitCash(wallet, action->costCents)) {
        return false;
    }
    adjustAgentOpinion(agentStore, action->targetAgentIndex, action->opinionBonus);
    CharacterAgentState* state = agentStore.states[action->targetAgentIndex].isActive ? &agentStore.states[action->targetAgentIndex] : nullptr;
    if (state != nullptr) {
        state->trust = std::clamp(state->trust + action->trustBonus, 0, 100);
        state->respect = std::clamp(state->respect + action->respectBonus, 0, 100);
        if (action->opinionBonus >= 4) {
            state->currentEmotion = AgentEmotion::Grateful;
        }
    }
    store.lastFamilyUpkeepTick = tickCount;
    return true;
}

} // namespace Core
