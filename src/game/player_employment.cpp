#include "game/player_employment.h"
#include "character/character_social_network.h"
#include "game/housing_living_costs.h"
#include "game/job_catalog.h"
#include "game/player_operations.h"
#include "sim/character_agent.h"
#include "world/business_node_table.h"
#include <algorithm>
#include <cstring>

namespace Core {

int32_t getPlayerJobCount(const PlayerOperationsStore& store) {
    int32_t count = 0;
    for (int i = 0; i < 2; ++i) {
        if (store.employedBusinessIndices[i] >= 0) {
            ++count;
        }
    }
    return count;
}

bool hasFullTimeJob(const PlayerOperationsStore& store) {
    for (int i = 0; i < 2; ++i) {
        if (store.employedBusinessIndices[i] >= 0) {
            const BusinessNodeDefinition* business = getBusinessNodeDefinition(store.employedBusinessIndices[i]);
            if (business != nullptr && business->scheduleType == JobScheduleType::FullTime) {
                return true;
            }
        }
    }
    return false;
}

bool canAcceptJob(const PlayerOperationsStore& store, JobScheduleType scheduleType) {
    const int32_t currentJobCount = getPlayerJobCount(store);
    if (currentJobCount >= 2) {
        return false;
    }
    if (scheduleType == JobScheduleType::FullTime && hasFullTimeJob(store)) {
        return false;
    }
    return true;
}

int32_t getJobSlotForNewHire(const PlayerOperationsStore& store, JobScheduleType scheduleType) {
    if (store.employedBusinessIndices[0] < 0) {
        return 0;
    }
    if (store.employedBusinessIndices[1] < 0) {
        return 1;
    }
    return -1;
}

bool isPlayerEmployed(const PlayerOperationsStore& store) {
    return store.employedBusinessIndices[0] >= 0 || store.employedBusinessIndices[1] >= 0;
}

bool canReapplyForJob(const PlayerOperationsStore& store, int32_t businessNodeIndex, uint64_t tickCount) {
    if (businessNodeIndex < 0 || businessNodeIndex >= MAX_BUSINESS_NODE_COUNT) {
        return false;
    }
    const uint64_t availableTick = store.jobReapplyAvailableTickByBusiness[businessNodeIndex];
    return tickCount >= availableTick;
}

uint64_t getJobReapplyTicksRemaining(
    const PlayerOperationsStore& store,
    int32_t businessNodeIndex,
    uint64_t tickCount) {
    if (businessNodeIndex < 0 || businessNodeIndex >= MAX_BUSINESS_NODE_COUNT) {
        return 0ULL;
    }
    const uint64_t availableTick = store.jobReapplyAvailableTickByBusiness[businessNodeIndex];
    if (tickCount >= availableTick) {
        return 0ULL;
    }
    return availableTick - tickCount;
}

void recordJobRejection(PlayerOperationsStore& store, int32_t businessNodeIndex, uint64_t tickCount) {
    if (businessNodeIndex < 0 || businessNodeIndex >= MAX_BUSINESS_NODE_COUNT) {
        return;
    }
    store.jobReapplyAvailableTickByBusiness[businessNodeIndex] = tickCount + JOB_REAPPLY_COOLDOWN_TICKS;
}

void spawnEmployerBossContact(CharacterAgentStore& store, int32_t businessNodeIndex) {
    const BusinessNodeDefinition* business = getBusinessNodeDefinition(businessNodeIndex);
    if (business == nullptr) {
        return;
    }
    CharacterAgentState& bossState = store.states[BOSS_AGENT_SLOT_INDEX];
    bossState = CharacterAgentState{};
    bossState.hasGeneratedIdentity = true;
    bossState.isActive = true;
    std::snprintf(bossState.generatedDisplayName, sizeof(bossState.generatedDisplayName), "%s", business->mapLabel);
    std::snprintf(bossState.generatedRoleLabel, sizeof(bossState.generatedRoleLabel), "Boss");
    bossState.opinionOfPlayer = 18;
    deriveRelationshipStatsFromOpinion(bossState);
    bossState.currentEmotion = AgentEmotion::Calm;
}

void clearEmployerBossContact(CharacterAgentStore& store) {
    store.states[BOSS_AGENT_SLOT_INDEX] = CharacterAgentState{};
}

bool tryHirePlayerAtBusiness(
    PlayerOperationsStore& store,
    const PlayerProfile& profile,
    CharacterAgentStore& agentStore,
    int32_t businessNodeIndex,
    int32_t interviewScore) {
    if (interviewScore < JOB_INTERVIEW_PASS_SCORE) {
        return false;
    }
    const BusinessNodeDefinition* business = getBusinessNodeDefinition(businessNodeIndex);
    if (business == nullptr) {
        return false;
    }
    if (!canAcceptJob(store, business->scheduleType)) {
        return false;
    }
    if (getNetworkAccessScore(profile) < business->minNetworkAccess) {
        return false;
    }
    const char* lockReason = nullptr;
    if (!evaluateJobEligibility(profile, store, businessNodeIndex, store.workExperienceMonths, 0ULL, lockReason)) {
        return false;
    }
    const int32_t slot = getJobSlotForNewHire(store, business->scheduleType);
    if (slot < 0) {
        return false;
    }
    store.employedBusinessIndices[slot] = businessNodeIndex;
    if (store.workExperienceMonths < 240) {
        store.workExperienceMonths += 1;
    }
    if (slot == 0) {
        spawnEmployerBossContact(agentStore, businessNodeIndex);
    }
    return true;
}

float computeEmployedLegitIncomePerTickCents(const PlayerOperationsStore& store) {
    float total = 0.0f;
    for (int i = 0; i < 2; ++i) {
        if (store.employedBusinessIndices[i] >= 0) {
            const BusinessNodeDefinition* business = getBusinessNodeDefinition(store.employedBusinessIndices[i]);
            if (business != nullptr) {
                float monthlyCents = static_cast<float>(computeBusinessMonthlyWageCents(*business)) * static_cast<float>(JOB_MONTHLY_WAGE_MULTIPLIER);
                if (business->scheduleType == JobScheduleType::PartTime) {
                    monthlyCents *= 0.6f;
                }
                total += monthlyCents / static_cast<float>(MONTHLY_LEDGER_INTERVAL_TICKS);
            }
        }
    }
    return total;
}

} // namespace Core
