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
    {12, {10, static_cast<uint32_t>(JobPerkFlags::TransitPass) | static_cast<uint32_t>(JobPerkFlags::MealAllowance), 2}},
    {13, {14, static_cast<uint32_t>(JobPerkFlags::HealthBenefits) | static_cast<uint32_t>(JobPerkFlags::UnionProtection), 3}},
    {14, {8, static_cast<uint32_t>(JobPerkFlags::TransitPass), 2}},
    {15, {1, static_cast<uint32_t>(JobPerkFlags::MealAllowance), 0}},
    {16, {2, static_cast<uint32_t>(JobPerkFlags::MealAllowance), 1}},
    {17, {3, static_cast<uint32_t>(JobPerkFlags::TransitPass), 1}},
    {18, {20, static_cast<uint32_t>(JobPerkFlags::HealthBenefits) | static_cast<uint32_t>(JobPerkFlags::UnionProtection), 3}},
    {19, {6, static_cast<uint32_t>(JobPerkFlags::UnionProtection), 2}},
    {20, {12, static_cast<uint32_t>(JobPerkFlags::HealthBenefits), 2}},
    {21, {9, static_cast<uint32_t>(JobPerkFlags::UnionProtection) | static_cast<uint32_t>(JobPerkFlags::MealAllowance), 2}},
};

constexpr int32_t JOB_EXTENSION_COUNT = static_cast<int32_t>(sizeof(JOB_EXTENSION_TABLE) / sizeof(JOB_EXTENSION_TABLE[0]));

constexpr const char* QUESTION_PROMPTS[JOB_INTERVIEW_QUESTION_BANK_SIZE] = {
    "Why do you want this job?",
    "How do you handle a difficult customer?",
    "What is your biggest weakness?",
    "A register is short at closing. What do you do?",
    "The boss asks you to stay late without pay. Your move?",
    "You see a coworker skimming. What happens next?",
    "How do you show up when the neighborhood is tense?",
    "A supplier offers you a kickback. Your response?",
    "You are blamed for a mistake you did not make.",
    "How do you treat regulars who know your name?",
    "Two coworkers ask you to cover conflicting favors.",
    "What would you do if business slows for a month?",
    "How do you handle a boss who micromanages?",
    "A friend asks you to bend policy for them.",
    "You find cash left behind at a table.",
    "How do you learn a task you have never done?",
    "What does loyalty to this workplace mean to you?",
    "How do you act when a rival insults the shop?",
    "You overhear talk about a strike or slowdown.",
    "Why should we trust you with the register?",
};

constexpr const char* ANSWER_VARIANT_GOOD[JOB_INTERVIEW_ANSWER_VARIANT_COUNT] = {
    "I stay honest, show up on time, and keep the books clean.",
    "I listen first, stay calm, and fix what I can without drama.",
    "I admit a real flaw and explain how I work around it.",
    "I report problems early and make the numbers right.",
    "I ask for fair comp or trade hours — no free favors.",
    "I keep my head down and do the job I was hired for.",
    "I treat customers fair even when the block is hot.",
    "I refuse side deals and stay on the straight wage.",
    "I take the write-up, learn, and do better next shift.",
    "I remember faces, stay polite, and protect the house.",
};

constexpr const char* ANSWER_VARIANT_NEUTRAL[JOB_INTERVIEW_ANSWER_VARIANT_COUNT] = {
    "I need steady pay and I will try to fit in.",
    "I get loud until the other person backs off.",
    "I say I have no weaknesses — I am reliable.",
    "I cover it tonight and fix it tomorrow quietly.",
    "I stay late if the family understands the sacrifice.",
    "I watch who is skimming and decide later.",
    "I keep to myself unless someone pushes me.",
    "I hear them out but do not commit in writing.",
    "I argue back but still clock my hours.",
    "I am friendly but do not get too personal.",
};

constexpr const char* ANSWER_VARIANT_POOR[JOB_INTERVIEW_ANSWER_VARIANT_COUNT] = {
    "I need cash fast and this job is cover.",
    "I threaten them until they leave the counter.",
    "I borrow from the till when things get tight.",
    "I split the shortage and hope nobody counts.",
    "I refuse extra work unless there is cash under the table.",
    "I take a cut if they keep quiet about my name.",
    "I lean on people I know to keep heat off me.",
    "I take the kickback if the boss never finds out.",
    "I blame someone else and walk off the shift.",
    "I charm them, then charge extra when they return.",
};

constexpr int32_t SCORE_GOOD_BY_QUESTION[JOB_INTERVIEW_QUESTION_BANK_SIZE] = {
    3, 3, 2, 4, 3, 1, 2, 4, 2, 3, 2, 3, 2, 3, 4, 3, 3, 2, 2, 4,
};
constexpr int32_t SCORE_NEUTRAL_BY_QUESTION[JOB_INTERVIEW_QUESTION_BANK_SIZE] = {
    1, 0, 1, -2, 0, -1, 0, 0, 0, 1, 0, 1, 0, 0, -1, 1, 1, 0, 0, 0,
};
constexpr int32_t SCORE_POOR_BY_QUESTION[JOB_INTERVIEW_QUESTION_BANK_SIZE] = {
    0, 1, -2, -3, 2, 0, 1, -3, -2, 0, -1, -1, -1, -2, -4, 0, -1, 1, -1, -3,
};

void copyAnswerVariant(char* destination, size_t destinationSize, const char* variantText) {
    std::snprintf(destination, destinationSize, "%s", variantText);
}

void shuffleQuestionOrder(int32_t* questionOrder, int32_t questionCount, uint64_t seed, int32_t businessIndex) {
    for (int32_t index = 0; index < questionCount; ++index) {
        questionOrder[index] = index;
    }
    for (int32_t index = questionCount - 1; index > 0; --index) {
        const uint32_t mix = Utils::hashSeedMix(
            Utils::hashSeedMix(seed, businessIndex, static_cast<int32_t>(index)),
            0x5149,
            0x5249U);
        const int32_t swapIndex = static_cast<int32_t>(mix % static_cast<uint32_t>(index + 1));
        const int32_t temp = questionOrder[index];
        questionOrder[index] = questionOrder[swapIndex];
        questionOrder[swapIndex] = temp;
    }
}

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
    uint64_t tickCount,
    const char*& outLockReason) {
    const BusinessNodeDefinition* business = getBusinessNodeDefinition(businessIndex);
    if (business == nullptr) {
        outLockReason = "Invalid workplace";
        return false;
    }
    if (isLawOfficeBusinessIndex(businessIndex)) {
        outLockReason = "Not a job site";
        return false;
    }
    if (getNetworkAccessScore(profile) < business->minNetworkAccess) {
        outLockReason = "Need more network access";
        return false;
    }
    if (business->preferredBackgroundId != 0
        && static_cast<uint8_t>(profile.draft.backgroundId) != business->preferredBackgroundId) {
        outLockReason = "This workplace favors another background";
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
    if (!canReapplyForJob(operationsStore, businessIndex, tickCount)) {
        outLockReason = "Reapply cooldown";
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
    const int32_t questionCount = std::clamp(
        JOB_INTERVIEW_MIN_QUESTIONS + (difficulty % 3),
        JOB_INTERVIEW_MIN_QUESTIONS,
        JOB_INTERVIEW_MAX_QUESTIONS);
    session.questionCount = questionCount;
    int32_t questionOrder[JOB_INTERVIEW_QUESTION_BANK_SIZE]{};
    shuffleQuestionOrder(questionOrder, JOB_INTERVIEW_QUESTION_BANK_SIZE, worldSeed, businessIndex);
    for (int32_t questionIndex = 0; questionIndex < session.questionCount; ++questionIndex) {
        const int32_t bankIndex = questionOrder[questionIndex];
        const uint32_t variantMix = Utils::hashSeedMix(
            Utils::hashSeedMix(worldSeed, businessIndex, bankIndex),
            0x5641,
            0x5254U);
        const int32_t variantIndex = static_cast<int32_t>(variantMix % static_cast<uint32_t>(JOB_INTERVIEW_ANSWER_VARIANT_COUNT));
        JobInterviewQuestionInstance& question = session.questions[questionIndex];
        std::snprintf(question.prompt, sizeof(question.prompt), "%s", QUESTION_PROMPTS[bankIndex]);
        copyAnswerVariant(question.answers[0].text, sizeof(question.answers[0].text), ANSWER_VARIANT_GOOD[variantIndex]);
        copyAnswerVariant(question.answers[1].text, sizeof(question.answers[1].text), ANSWER_VARIANT_NEUTRAL[variantIndex]);
        copyAnswerVariant(question.answers[2].text, sizeof(question.answers[2].text), ANSWER_VARIANT_POOR[variantIndex]);
        question.answers[0].scoreDelta = SCORE_GOOD_BY_QUESTION[bankIndex];
        question.answers[1].scoreDelta = SCORE_NEUTRAL_BY_QUESTION[bankIndex];
        question.answers[2].scoreDelta = SCORE_POOR_BY_QUESTION[bankIndex];
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
