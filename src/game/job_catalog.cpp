#include "game/job_catalog.h"
#include "game/player_employment.h"
#include "utils/seed_hash.h"
#include <algorithm>
#include <cstring>

namespace Core {

namespace {

struct JobExtensionEntry {
    int32_t businessIndex;
    JobDefinitionExtension extension;
};

constexpr JobExtensionEntry JOB_EXTENSION_TABLE[] = {
    {0, {0, static_cast<uint32_t>(JobPerkFlags::MealAllowance), 1}},
    {1, {3, static_cast<uint32_t>(JobPerkFlags::TransitPass), 1}},
    {2, {0, static_cast<uint32_t>(JobPerkFlags::None), 0}},
    {3, {6, static_cast<uint32_t>(JobPerkFlags::UnionProtection) | static_cast<uint32_t>(JobPerkFlags::HealthBenefits), 2}},
    {4, {12, static_cast<uint32_t>(JobPerkFlags::HealthBenefits), 2}},
    {5, {2, static_cast<uint32_t>(JobPerkFlags::MealAllowance), 1}},
    {6, {8, static_cast<uint32_t>(JobPerkFlags::TransitPass) | static_cast<uint32_t>(JobPerkFlags::UnionProtection), 2}},
    {7, {1, static_cast<uint32_t>(JobPerkFlags::None), 0}},
    {8, {18, static_cast<uint32_t>(JobPerkFlags::HealthBenefits) | static_cast<uint32_t>(JobPerkFlags::MealAllowance), 3}},
    {9, {4, static_cast<uint32_t>(JobPerkFlags::TransitPass), 1}},
    {10, {6, static_cast<uint32_t>(JobPerkFlags::MealAllowance), 2}},
    {11, {0, static_cast<uint32_t>(JobPerkFlags::None), 0}},
};

constexpr int32_t JOB_EXTENSION_COUNT = static_cast<int32_t>(sizeof(JOB_EXTENSION_TABLE) / sizeof(JOB_EXTENSION_TABLE[0]));

constexpr const char* QUESTION_PROMPTS[] = {
    "Why do you want this job?",
    "How do you handle a difficult customer?",
    "What is your biggest weakness?",
    "A register is short at closing. What do you do?",
    "The boss asks you to stay late without pay. Your move?",
    "You see a coworker skimming. What happens next?",
    "How do you show up when the neighborhood is tense?",
};

constexpr const char* ANSWER_SET_A[] = {
    "I need steady pay and I will show up on time.",
    "Stay calm, listen, and fix what I can.",
    "I talk too much when I am nervous.",
    "Report it immediately and make the books whole.",
    "I ask for comp time or a straight trade next week.",
    "I keep my head down unless it touches my crew.",
    "I keep my business quiet and do my work.",
};
constexpr const char* ANSWER_SET_B[] = {
    "My cousin said you hire anyone.",
    "I yell back until they leave.",
    "I have no weaknesses.",
    "Cover it tonight and fix it tomorrow.",
    "Sure, family comes second to the job.",
    "I take a cut if they share.",
    "I lean on people I know to keep heat off.",
};
constexpr const char* ANSWER_SET_C[] = {
    "I am between rackets and need cover.",
    "I know a guy who handles problems.",
    "I sometimes borrow from the till.",
    "Split the difference and move on.",
    "I refuse unless there is cash on the table.",
    "I tell the boss if the price is right.",
    "I make sure the right people vouch for me.",
};

constexpr int32_t SCORE_A[] = {3, 3, 2, 4, 3, 1, 2};
constexpr int32_t SCORE_B[] = {1, 0, 1, -2, 0, -1, 0};
constexpr int32_t SCORE_C[] = {0, 1, -2, -3, 2, 0, 1};

const JobDefinitionExtension* findExtension(int32_t businessIndex) {
    for (int32_t entryIndex = 0; entryIndex < JOB_EXTENSION_COUNT; ++entryIndex) {
        if (JOB_EXTENSION_TABLE[entryIndex].businessIndex == businessIndex) {
            return &JOB_EXTENSION_TABLE[entryIndex].extension;
        }
    }
    return nullptr;
}

} // namespace

const JobDefinitionExtension* getJobDefinitionExtension(int32_t businessIndex) {
    return findExtension(businessIndex);
}

bool evaluateJobEligibility(
    const PlayerProfile& profile,
    const PlayerOperationsStore& operationsStore,
    int32_t businessIndex,
    int32_t playerWorkExperienceMonths,
    const char*& outLockReason) {
    const BusinessNodeDefinition* business = getBusinessNodeDefinition(businessIndex);
    if (business == nullptr) {
        outLockReason = "Invalid workplace";
        return false;
    }
    if (getNetworkAccessScore(profile) < business->minNetworkAccess) {
        outLockReason = "Need more network access";
        return false;
    }
    const JobDefinitionExtension* extension = findExtension(businessIndex);
    if (extension != nullptr && playerWorkExperienceMonths < extension->minWorkExperienceMonths) {
        outLockReason = "Need more work experience";
        return false;
    }
    if (isPlayerEmployed(operationsStore)) {
        outLockReason = "Already employed";
        return false;
    }
    outLockReason = nullptr;
    return true;
}

void buildJobInterviewSession(JobInterviewSession& session, int32_t businessIndex, uint64_t worldSeed) {
    session.businessNodeIndex = businessIndex;
    session.currentQuestionIndex = 0;
    session.totalScore = 0;
    const JobDefinitionExtension* extension = findExtension(businessIndex);
    const int32_t difficulty = extension != nullptr ? extension->interviewDifficulty : 1;
    session.questionCount = std::min(JOB_INTERVIEW_MAX_QUESTIONS, 3 + difficulty);
    for (int32_t questionIndex = 0; questionIndex < session.questionCount; ++questionIndex) {
        const int32_t promptIndex = static_cast<int32_t>(Utils::hashSeedMix(worldSeed, businessIndex, questionIndex) % 7U);
        JobInterviewQuestionInstance& question = session.questions[questionIndex];
        std::snprintf(question.prompt, sizeof(question.prompt), "%s", QUESTION_PROMPTS[promptIndex]);
        std::snprintf(question.answers[0].text, sizeof(question.answers[0].text), "%s", ANSWER_SET_A[promptIndex]);
        std::snprintf(question.answers[1].text, sizeof(question.answers[1].text), "%s", ANSWER_SET_B[promptIndex]);
        std::snprintf(question.answers[2].text, sizeof(question.answers[2].text), "%s", ANSWER_SET_C[promptIndex]);
        question.answers[0].scoreDelta = SCORE_A[promptIndex];
        question.answers[1].scoreDelta = SCORE_B[promptIndex];
        question.answers[2].scoreDelta = SCORE_C[promptIndex];
    }
}

bool evaluateJobInterviewPass(const JobInterviewSession& session, int32_t businessIndex) {
    const JobDefinitionExtension* extension = findExtension(businessIndex);
    const int32_t difficulty = extension != nullptr ? extension->interviewDifficulty : 1;
    const int32_t passScore = JOB_INTERVIEW_PASS_SCORE + difficulty;
    return session.totalScore >= passScore;
}

const char* jobPerkFlagsToShortLabel(uint32_t perkFlags) {
    static char labelBuffer[96];
    labelBuffer[0] = '\0';
    if ((perkFlags & static_cast<uint32_t>(JobPerkFlags::HealthBenefits)) != 0U) {
        std::strncat(labelBuffer, "Health benefits; ", sizeof(labelBuffer) - 1);
    }
    if ((perkFlags & static_cast<uint32_t>(JobPerkFlags::TransitPass)) != 0U) {
        std::strncat(labelBuffer, "Transit pass; ", sizeof(labelBuffer) - 1);
    }
    if ((perkFlags & static_cast<uint32_t>(JobPerkFlags::MealAllowance)) != 0U) {
        std::strncat(labelBuffer, "Meal allowance; ", sizeof(labelBuffer) - 1);
    }
    if ((perkFlags & static_cast<uint32_t>(JobPerkFlags::UnionProtection)) != 0U) {
        std::strncat(labelBuffer, "Union protection; ", sizeof(labelBuffer) - 1);
    }
    if (labelBuffer[0] == '\0') {
        std::snprintf(labelBuffer, sizeof(labelBuffer), "No listed perks");
    }
    return labelBuffer;
}

} // namespace Core
