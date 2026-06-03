#include "game/player_employment.h"
#include "game/housing_living_costs.h"
#include "game/player_operations.h"
#include "world/business_node_table.h"

namespace Core {

bool isPlayerEmployed(const PlayerOperationsStore& store) {
    return store.employedBusinessIndex >= 0;
}

bool tryHirePlayerAtBusiness(
    PlayerOperationsStore& store,
    const PlayerProfile& profile,
    int32_t businessNodeIndex,
    int32_t interviewScore) {
    if (isPlayerEmployed(store)) {
        return false;
    }
    if (interviewScore < JOB_INTERVIEW_PASS_SCORE) {
        return false;
    }
    const BusinessNodeDefinition* business = getBusinessNodeDefinition(businessNodeIndex);
    if (business == nullptr) {
        return false;
    }
    if (getNetworkAccessScore(profile) < business->minNetworkAccess) {
        return false;
    }
    store.employedBusinessIndex = businessNodeIndex;
    return true;
}

float computeEmployedLegitIncomePerTickCents(const PlayerOperationsStore& store) {
    if (!isPlayerEmployed(store)) {
        return 0.0f;
    }
    const BusinessNodeDefinition* business = getBusinessNodeDefinition(store.employedBusinessIndex);
    if (business == nullptr) {
        return 0.0f;
    }
    const float monthlyCents = static_cast<float>(business->jobWageCents) * static_cast<float>(JOB_MONTHLY_WAGE_MULTIPLIER);
    return monthlyCents / static_cast<float>(MONTHLY_LEDGER_INTERVAL_TICKS);
}

} // namespace Core
