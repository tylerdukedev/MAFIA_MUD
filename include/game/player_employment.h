#pragma once

#include "character/player_profile.h"
#include "game/player_operations.h"
#include "game/player_wallet.h"
#include "sim/character_agent.h"
#include <cstdint>

namespace Core {

constexpr int32_t JOB_INTERVIEW_PASS_SCORE = 5;
constexpr uint64_t JOB_REAPPLY_COOLDOWN_TICKS = 240;

int32_t getPlayerJobCount(const PlayerOperationsStore& store);
bool hasFullTimeJob(const PlayerOperationsStore& store);
bool canAcceptJob(const PlayerOperationsStore& store, JobScheduleType scheduleType);
int32_t getJobSlotForNewHire(const PlayerOperationsStore& store, JobScheduleType scheduleType);
bool isPlayerEmployed(const PlayerOperationsStore& store);
bool tryHirePlayerAtBusiness(
    PlayerOperationsStore& store,
    const PlayerProfile& profile,
    CharacterAgentStore& agentStore,
    int32_t businessNodeIndex,
    int32_t interviewScore);
float computeEmployedLegitIncomePerTickCents(const PlayerOperationsStore& store);
bool canReapplyForJob(const PlayerOperationsStore& store, int32_t businessNodeIndex, uint64_t tickCount);
uint64_t getJobReapplyTicksRemaining(const PlayerOperationsStore& store, int32_t businessNodeIndex, uint64_t tickCount);
void recordJobRejection(PlayerOperationsStore& store, int32_t businessNodeIndex, uint64_t tickCount);
void spawnEmployerBossContact(CharacterAgentStore& store, int32_t businessNodeIndex);
void clearEmployerBossContact(CharacterAgentStore& store);

} // namespace Core
