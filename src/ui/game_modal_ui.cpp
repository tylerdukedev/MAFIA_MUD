#include "ui/game_modal_ui.h"
#include "game/landlord_contact.h"
#include "game/player_employment.h"
#include "game/operation_types.h"
#include "game/player_operations.h"
#include "world/business_node_table.h"
#include "imgui.h"
#include <cstdio>
#include <cstring>

namespace Core {

namespace {

constexpr int32_t JOB_INTERVIEW_QUESTION_COUNT = 3;

struct JobInterviewQuestion {
    const char* prompt;
    const char* answerA;
    const char* answerB;
    const char* answerC;
    int32_t scoreA;
    int32_t scoreB;
    int32_t scoreC;
};

constexpr JobInterviewQuestion JOB_INTERVIEW_QUESTIONS[JOB_INTERVIEW_QUESTION_COUNT] = {
    {"Why do you want this job?",
     "I need steady pay and I will show up on time.",
     "My cousin said you hire anyone.",
     "I am between rackets and need cover.",
     3, 1, 0},
    {"How do you handle a difficult customer?",
     "Stay calm, listen, and fix what I can.",
     "I yell back until they leave.",
     "I know a guy who handles problems.",
     3, 0, 1},
    {"What is your biggest weakness?",
     "I talk too much when I am nervous.",
     "I have no weaknesses.",
     "I sometimes borrow from the till.",
     2, 1, -2},
};

void closeModal(GameModalState& modal, SimClock& simClock) {
    resetGameModalState(modal);
    simClock.setPaused(false);
}

void setModalStatus(GameModalState& modal, const char* message) {
    std::strncpy(modal.statusMessage, message, sizeof(modal.statusMessage) - 1);
    modal.statusMessage[sizeof(modal.statusMessage) - 1] = '\0';
}

} // namespace

void beginJobInterviewModal(GameModalState& modal, int32_t businessNodeIndex, SimClock& simClock) {
    resetGameModalState(modal);
    modal.isActive = true;
    modal.kind = GameModalKind::JobInterview;
    modal.pauseSimulation = true;
    modal.lockOtherPanels = true;
    modal.businessNodeIndex = businessNodeIndex;
    modal.interviewQuestionIndex = 0;
    modal.interviewScore = 0;
    modal.selectedAnswerIndex = -1;
    setModalStatus(modal, "Job interview in progress.");
    simClock.setPaused(true);
}

void beginApartmentApplicationModal(GameModalState& modal, int32_t catalogIndex, SimClock& simClock) {
    resetGameModalState(modal);
    modal.isActive = true;
    modal.kind = GameModalKind::ApartmentApplication;
    modal.pauseSimulation = true;
    modal.lockOtherPanels = true;
    modal.businessNodeIndex = catalogIndex;
    modal.selectedAnswerIndex = -1;
    setModalStatus(modal, "Rental application — answer honestly or bluff.");
    simClock.setPaused(true);
}

void beginWorkDayCommuteModal(GameModalState& modal, bool isLateForWork, SimClock& simClock) {
    resetGameModalState(modal);
    modal.isActive = true;
    modal.kind = GameModalKind::WorkDayCommute;
    modal.pauseSimulation = true;
    modal.lockOtherPanels = true;
    modal.isLateForWork = isLateForWork;
    setModalStatus(modal, isLateForWork ? "You are running late for your shift." : "Your shift starts soon.");
    simClock.setPaused(true);
}

void tickWorkDayCommutePrompt(
    GameModalState& modal,
    PlayerWorldState& playerWorldState,
    const PlayerOperationsStore& playerOperationsStore,
    SimClock& simClock,
    uint64_t tickCount) {
    if (modal.isActive || !isPlayerEmployed(playerOperationsStore) || !playerWorldState.isOnWorkShiftToday) {
        return;
    }
    if (tickCount < playerWorldState.lastWorkDayPromptTick + static_cast<uint64_t>(WORK_DAY_INTERVAL_TICKS)) {
        return;
    }
    playerWorldState.lastWorkDayPromptTick = tickCount;
    const bool isLate = (tickCount % static_cast<uint64_t>(WORK_DAY_INTERVAL_TICKS)) > static_cast<uint64_t>(WORK_DAY_INTERVAL_TICKS / 4);
    beginWorkDayCommuteModal(modal, isLate, simClock);
}

void renderGameModalOverlay(
    GameModalState& modal,
    SimClock& simClock,
    PlayerOperationsStore& playerOperationsStore,
    PlayerWallet& playerWallet,
    PlayerWorldState& playerWorldState,
    CharacterAgentStore& characterAgentStore,
    SimEventQueue& simEventQueue,
    const PlayerProfile& playerProfile,
    uint64_t tickCount) {
    (void)simEventQueue;
    (void)tickCount;
    if (!modal.isActive) {
        return;
    }
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    const ImVec2 center(viewport->WorkPos.x + viewport->WorkSize.x * 0.5f, viewport->WorkPos.y + viewport->WorkSize.y * 0.5f);
    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(520.0f, 360.0f), ImGuiCond_Always);
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    if (!ImGui::Begin("Event", nullptr, flags)) {
        ImGui::End();
        return;
    }
    if (modal.kind == GameModalKind::JobInterview) {
        const BusinessNodeDefinition* business = getBusinessNodeDefinition(modal.businessNodeIndex);
        ImGui::Text("Job Interview");
        if (business != nullptr) {
            ImGui::TextDisabled("%s", business->fullName);
        }
        ImGui::Separator();
        if (modal.interviewQuestionIndex >= JOB_INTERVIEW_QUESTION_COUNT) {
            const bool hired = tryHirePlayerAtBusiness(
                playerOperationsStore, playerProfile, modal.businessNodeIndex, modal.interviewScore);
            if (hired) {
                playerWorldState.isEmployed = true;
                setModalStatus(modal, "You are hired. Wages accrue while employed.");
            } else {
                setModalStatus(modal, "They passed on you this time.");
            }
            ImGui::TextWrapped("%s", modal.statusMessage);
            if (ImGui::Button("Close", ImVec2(160.0f, 0.0f))) {
                closeModal(modal, simClock);
            }
        } else {
            const JobInterviewQuestion& question = JOB_INTERVIEW_QUESTIONS[modal.interviewQuestionIndex];
            ImGui::TextWrapped("%s", question.prompt);
            ImGui::Spacing();
            if (ImGui::Selectable(question.answerA, modal.selectedAnswerIndex == 0)) {
                modal.selectedAnswerIndex = 0;
            }
            if (ImGui::Selectable(question.answerB, modal.selectedAnswerIndex == 1)) {
                modal.selectedAnswerIndex = 1;
            }
            if (ImGui::Selectable(question.answerC, modal.selectedAnswerIndex == 2)) {
                modal.selectedAnswerIndex = 2;
            }
            if (modal.selectedAnswerIndex >= 0 && ImGui::Button("Submit answer", ImVec2(180.0f, 0.0f))) {
                if (modal.selectedAnswerIndex == 0) {
                    modal.interviewScore += question.scoreA;
                } else if (modal.selectedAnswerIndex == 1) {
                    modal.interviewScore += question.scoreB;
                } else {
                    modal.interviewScore += question.scoreC;
                }
                modal.interviewQuestionIndex += 1;
                modal.selectedAnswerIndex = -1;
            }
        }
    } else if (modal.kind == GameModalKind::ApartmentApplication) {
        ImGui::Text("Apartment Rental Application");
        ImGui::Separator();
        ImGui::TextWrapped("The landlord asks about income and references. Your answers are binding.");
        ImGui::Spacing();
        if (ImGui::Selectable("Truthful: modest savings, no steady job yet", modal.selectedAnswerIndex == 0)) {
            modal.selectedAnswerIndex = 0;
        }
        if (ImGui::Selectable("Lie: claim high wages from a job you do not have", modal.selectedAnswerIndex == 1)) {
            modal.selectedAnswerIndex = 1;
        }
        if (ImGui::Selectable("Lie: claim family vouches for you (risky if untrue)", modal.selectedAnswerIndex == 2)) {
            modal.selectedAnswerIndex = 2;
        }
        if (modal.selectedAnswerIndex >= 0 && ImGui::Button("Submit application", ImVec2(200.0f, 0.0f))) {
            const int32_t catalogIndex = modal.businessNodeIndex;
            bool approved = false;
            if (modal.selectedAnswerIndex == 0) {
                approved = true;
            } else if (modal.selectedAnswerIndex == 1) {
                approved = isPlayerEmployed(playerOperationsStore);
            } else if (modal.selectedAnswerIndex == 2) {
                approved = playerProfile.draft.hasFamilyInCountry || playerProfile.draft.hasFriendsInCountry;
            }
            if (approved && tryEstablishOperation(playerOperationsStore, playerWallet, playerProfile, catalogIndex, tickCount)) {
                spawnLandlordContact(characterAgentStore);
                playerWorldState.hasLandlordContact = true;
                setModalStatus(modal, "Approved. Morris Schwartz is now a contact.");
            } else if (!approved) {
                setModalStatus(modal, "Denied — your story did not hold up.");
            } else {
                setModalStatus(modal, "Could not complete move-in (cash or requirements).");
            }
            modal.interviewQuestionIndex = 1;
        }
        if (modal.interviewQuestionIndex > 0) {
            ImGui::TextWrapped("%s", modal.statusMessage);
            if (ImGui::Button("Close", ImVec2(160.0f, 0.0f))) {
                closeModal(modal, simClock);
            }
        }
    } else if (modal.kind == GameModalKind::WorkDayCommute) {
        ImGui::Text("Work Day");
        ImGui::Separator();
        ImGui::TextWrapped("%s", modal.statusMessage);
        if (ImGui::Button("Go to work (on time)", ImVec2(200.0f, 0.0f))) {
            playerWorldState.isAtWork = true;
            playerWorldState.isOnWorkShiftToday = false;
            closeModal(modal, simClock);
        }
        ImGui::SameLine();
        if (ImGui::Button("Go to work (late)", ImVec2(200.0f, 0.0f))) {
            playerWorldState.isAtWork = true;
            playerWorldState.isOnWorkShiftToday = false;
            closeModal(modal, simClock);
        }
        if (ImGui::Button("Call out / skip shift", ImVec2(200.0f, 0.0f))) {
            playerWorldState.isAtWork = false;
            playerWorldState.isOnWorkShiftToday = false;
            closeModal(modal, simClock);
        }
    }
    ImGui::End();
}

} // namespace Core
