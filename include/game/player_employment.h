#pragma once

#include "character/player_profile.h"
#include "game/player_operations.h"
#include "game/player_wallet.h"
#include <cstdint>

namespace Core {

constexpr int32_t JOB_INTERVIEW_PASS_SCORE = 5;
constexpr int32_t JOB_MONTHLY_WAGE_MULTIPLIER = 10;

bool isPlayerEmployed(const PlayerOperationsStore& store);
bool tryHirePlayerAtBusiness(
    PlayerOperationsStore& store,
    const PlayerProfile& profile,
    int32_t businessNodeIndex,
    int32_t interviewScore);
float computeEmployedLegitIncomePerTickCents(const PlayerOperationsStore& store);

} // namespace Core
