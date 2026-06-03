#pragma once

#include "character/player_profile.h"
#include "game/player_operations.h"
#include "world/business_node_table.h"
#include <cstdint>

namespace Core {

constexpr int32_t JOB_INTERVIEW_MAX_QUESTIONS = 5;
constexpr int32_t JOB_INTERVIEW_ANSWER_COUNT = 3;

enum class JobPerkFlags : uint32_t {
    None = 0,
    HealthBenefits = 1U << 0,
    TransitPass = 1U << 1,
    MealAllowance = 1U << 2,
    UnionProtection = 1U << 3,
};

struct JobDefinitionExtension {
    int32_t minWorkExperienceMonths = 0;
    uint32_t perkFlags = 0;
    int32_t interviewDifficulty = 1;
};

struct JobInterviewAnswerOption {
    char text[96];
    int32_t scoreDelta = 0;
};

struct JobInterviewQuestionInstance {
    char prompt[128];
    JobInterviewAnswerOption answers[JOB_INTERVIEW_ANSWER_COUNT];
};

struct JobInterviewSession {
    int32_t businessNodeIndex = -1;
    int32_t questionCount = 0;
    int32_t currentQuestionIndex = 0;
    int32_t totalScore = 0;
    JobInterviewQuestionInstance questions[JOB_INTERVIEW_MAX_QUESTIONS]{};
};

const JobDefinitionExtension* getJobDefinitionExtension(int32_t businessIndex);
bool evaluateJobEligibility(
    const PlayerProfile& profile,
    const PlayerOperationsStore& operationsStore,
    int32_t businessIndex,
    int32_t playerWorkExperienceMonths,
    const char*& outLockReason);
void buildJobInterviewSession(JobInterviewSession& session, int32_t businessIndex, uint64_t worldSeed);
bool evaluateJobInterviewPass(const JobInterviewSession& session, int32_t businessIndex);
const char* jobPerkFlagsToShortLabel(uint32_t perkFlags);

} // namespace Core
