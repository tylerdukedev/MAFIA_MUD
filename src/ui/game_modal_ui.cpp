#include "ui/game_modal_ui.h"
#include "character/character_social_network.h"
#include "game/landlord_contact.h"
#include "game/player_employment.h"
#include "game/player_law_enforcement.h"
#include "game/player_organization.h"
#include "game/player_organization_ui.h"
#include "game/operation_types.h"
#include "game/player_operations.h"
#include "sim/character_agent.h"
#include "world/business_node_table.h"
#include "imgui.h"
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

void openModal(GameModalState& modal, SimClock& simClock, GameModalKind kind, const char* statusMessage) {
    resetGameModalState(modal);
    modal.isActive = true;
    modal.kind = kind;
    modal.pauseSimulation = true;
    modal.lockOtherPanels = true;
    setModalStatus(modal, statusMessage);
    simClock.setPaused(true);
}

bool evaluateApartmentApplicationApproval(const PlayerProfile& playerProfile, const PlayerOperationsStore& store, int32_t answerIndex) {
    if (answerIndex == 0) {
        return true;
    }
    if (answerIndex == 1) {
        return isPlayerEmployed(store);
    }
    if (answerIndex == 2) {
        return playerProfile.draft.hasFamilyInCountry || playerProfile.draft.hasFriendsInCountry;
    }
    return false;
}

void applyApartmentLieConsequences(CharacterAgentStore& agentStore, int32_t answerIndex, bool wasApproved) {
    if (!wasApproved || answerIndex == 0) {
        return;
    }
    const int32_t landlordSlot = FIRST_COMMUNITY_AGENT_SLOT_INDEX;
    if (!agentStore.states[landlordSlot].isActive) {
        return;
    }
    int32_t opinionDelta = -6;
    if (answerIndex == 1) {
        opinionDelta = -10;
    }
    adjustAgentOpinion(agentStore, landlordSlot, opinionDelta);
}

} // namespace

void beginJobInterviewModal(GameModalState& modal, int32_t businessNodeIndex, SimClock& simClock) {
    openModal(modal, simClock, GameModalKind::JobInterview, "Job interview in progress.");
    modal.businessNodeIndex = businessNodeIndex;
}

void beginApartmentApplicationModal(GameModalState& modal, int32_t catalogIndex, SimClock& simClock) {
    openModal(modal, simClock, GameModalKind::ApartmentApplication, "Rental application — answer honestly or bluff.");
    modal.businessNodeIndex = catalogIndex;
}

void beginWorkDayCommuteModal(GameModalState& modal, bool isLateForWork, SimClock& simClock) {
    openModal(modal, simClock, GameModalKind::WorkDayCommute, isLateForWork ? "You are running late for your shift." : "Your shift starts soon.");
    modal.isLateForWork = isLateForWork;
}

void beginCrewRecruitmentModal(GameModalState& modal, int32_t agentIndex, SimClock& simClock) {
    openModal(modal, simClock, GameModalKind::CrewRecruitment, "Make your pitch — will they run with you?");
    modal.targetAgentIndex = agentIndex;
}

void beginCrewFormalizeModal(GameModalState& modal, SimClock& simClock) {
    openModal(modal, simClock, GameModalKind::CrewFormalize, "Name your street crew.");
    std::snprintf(modal.crewNameBuffer, sizeof(modal.crewNameBuffer), "%s", "The Corner Crew");
}

void beginOrganizationCreationModal(GameModalState& modal, SimClock& simClock) {
    openModal(modal, simClock, GameModalKind::OrganizationCreation, "Incorporate — this makes you official on paper.");
    std::snprintf(modal.organizationNameBuffer, sizeof(modal.organizationNameBuffer), "%s", "Russo & Associates");
    std::snprintf(modal.organizationFrontBuffer, sizeof(modal.organizationFrontBuffer), "%s", "Import & Storage");
}

void tickWorkDayCommutePrompt(
    GameModalState& modal,
    PlayerWorldState& playerWorldState,
    const PlayerOperationsStore& playerOperationsStore,
    SimClock& simClock,
    uint64_t tickCount) {
    constexpr uint64_t WORK_SHIFT_DURATION_TICKS = static_cast<uint64_t>(WORK_DAY_INTERVAL_TICKS / 2);
    if (playerWorldState.isAtWork && playerWorldState.lastWorkDayPromptTick > 0
        && tickCount >= playerWorldState.lastWorkDayPromptTick + WORK_SHIFT_DURATION_TICKS) {
        playerWorldState.isAtWork = false;
    }
    if (modal.isActive || !isPlayerEmployed(playerOperationsStore) || playerWorldState.isAtWork) {
        return;
    }
    if (playerWorldState.lastWorkDayPromptTick > 0
        && tickCount < playerWorldState.lastWorkDayPromptTick + static_cast<uint64_t>(WORK_DAY_INTERVAL_TICKS)) {
        return;
    }
    const bool isLate = (tickCount % static_cast<uint64_t>(WORK_DAY_INTERVAL_TICKS)) > static_cast<uint64_t>(WORK_DAY_INTERVAL_TICKS / 4);
    beginWorkDayCommuteModal(modal, isLate, simClock);
}

void renderGameModalOverlay(
    GameModalState& modal,
    SimClock& simClock,
    PlayerOperationsStore& playerOperationsStore,
    PlayerOrganizationStore& playerOrganizationStore,
    PlayerLawEnforcementStore& playerLawEnforcementStore,
    PlayerWallet& playerWallet,
    PlayerWorldState& playerWorldState,
    CharacterAgentStore& characterAgentStore,
    SimEventQueue& simEventQueue,
    const PlayerProfile& playerProfile,
    uint64_t tickCount,
    uint64_t worldSeed) {
    (void)simEventQueue;
    if (!modal.isActive) {
        return;
    }
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    const ImVec2 center(viewport->WorkPos.x + viewport->WorkSize.x * 0.5f, viewport->WorkPos.y + viewport->WorkSize.y * 0.5f);
    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    const ImVec2 modalSize = modal.kind == GameModalKind::OrganizationCreation ? ImVec2(580.0f, 420.0f) : ImVec2(520.0f, 360.0f);
    ImGui::SetNextWindowSize(modalSize, ImGuiCond_Always);
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
            if (!modal.hasFlowResult) {
                const bool hired = tryHirePlayerAtBusiness(
                    playerOperationsStore, playerProfile, modal.businessNodeIndex, modal.interviewScore);
                setModalStatus(modal, hired ? "You are hired. Wages accrue while employed." : "They passed on you this time.");
                modal.hasFlowResult = true;
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
        if (modal.hasFlowResult) {
            ImGui::TextWrapped("%s", modal.statusMessage);
            if (ImGui::Button("Close", ImVec2(160.0f, 0.0f))) {
                closeModal(modal, simClock);
            }
        } else {
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
                const int32_t answerIndex = modal.selectedAnswerIndex;
                const bool approved = evaluateApartmentApplicationApproval(playerProfile, playerOperationsStore, answerIndex);
                if (approved && tryEstablishOperation(playerOperationsStore, playerWallet, playerProfile, catalogIndex, tickCount)) {
                    spawnLandlordContact(characterAgentStore);
                    playerWorldState.hasLandlordContact = true;
                    applyApartmentLieConsequences(characterAgentStore, answerIndex, true);
                    setModalStatus(modal, "Approved. Morris Schwartz is now a contact.");
                } else if (!approved) {
                    setModalStatus(modal, "Denied — your story did not hold up.");
                } else {
                    setModalStatus(modal, "Could not complete move-in (cash or requirements).");
                }
                modal.hasFlowResult = true;
            }
        }
    } else if (modal.kind == GameModalKind::WorkDayCommute) {
        ImGui::Text("Work Day");
        ImGui::Separator();
        ImGui::TextWrapped("%s", modal.statusMessage);
        if (ImGui::Button("Go to work (on time)", ImVec2(200.0f, 0.0f))) {
            playerWorldState.isAtWork = true;
            playerWorldState.lastWorkDayPromptTick = tickCount;
            modal.isLateForWork = false;
            closeModal(modal, simClock);
        }
        ImGui::SameLine();
        if (ImGui::Button("Go to work (late)", ImVec2(200.0f, 0.0f))) {
            playerWorldState.isAtWork = true;
            playerWorldState.lastWorkDayPromptTick = tickCount;
            modal.isLateForWork = true;
            setModalStatus(modal, "You clocked in late. Expect reduced pay until travel rules expand.");
            closeModal(modal, simClock);
        }
        if (ImGui::Button("Call out / skip shift", ImVec2(200.0f, 0.0f))) {
            playerWorldState.isAtWork = false;
            playerWorldState.lastWorkDayPromptTick = tickCount;
            closeModal(modal, simClock);
        }
    } else if (modal.kind == GameModalKind::CrewRecruitment) {
        const char* displayName = nullptr;
        const char* roleLabel = nullptr;
        tryGetAgentDisplayLabels(characterAgentStore, modal.targetAgentIndex, displayName, roleLabel);
        ImGui::Text("Recruit to crew");
        if (displayName != nullptr) {
            ImGui::TextDisabled("%s (%s)", displayName, roleLabel);
        }
        ImGui::Separator();
        if (modal.hasFlowResult) {
            ImGui::TextWrapped("%s", modal.statusMessage);
            if (ImGui::Button("Close", ImVec2(160.0f, 0.0f))) {
                closeModal(modal, simClock);
            }
        } else {
            ImGui::TextWrapped("Offer them a place in your street crew. Trust and respect matter.");
            if (ImGui::Selectable("Split fair — we eat together, share risk", modal.selectedAnswerIndex == 0)) {
                modal.selectedAnswerIndex = 0;
            }
            if (ImGui::Selectable("You cover muscle, I cover planning", modal.selectedAnswerIndex == 1)) {
                modal.selectedAnswerIndex = 1;
            }
            if (ImGui::Selectable("Heavy hand — my word is law", modal.selectedAnswerIndex == 2)) {
                modal.selectedAnswerIndex = 2;
            }
            if (modal.selectedAnswerIndex >= 0 && ImGui::Button("Make the offer", ImVec2(180.0f, 0.0f))) {
                const CharacterAgentState* agentState = getCharacterAgentState(characterAgentStore, modal.targetAgentIndex);
                if (agentState == nullptr) {
                    setModalStatus(modal, "They are not available.");
                    modal.hasFlowResult = true;
                } else {
                    const int32_t acceptChance = rollCrewRecruitAcceptChance(*agentState, playerProfile, modal.selectedAnswerIndex, worldSeed, modal.targetAgentIndex);
                    const uint32_t roll = static_cast<uint32_t>((worldSeed ^ tickCount ^ static_cast<uint64_t>(modal.targetAgentIndex)) % 100ULL);
                    if (static_cast<int32_t>(roll) < acceptChance && tryAddCrewMember(playerOrganizationStore, modal.targetAgentIndex)) {
                        setModalStatus(modal, "They accepted — added to your crew roster.");
                    } else {
                        setModalStatus(modal, "They passed for now. Build trust and try again later.");
                        adjustAgentOpinion(characterAgentStore, modal.targetAgentIndex, -3);
                    }
                    modal.hasFlowResult = true;
                }
            }
        }
    } else if (modal.kind == GameModalKind::CrewFormalize) {
        ImGui::Text("Formalize crew");
        ImGui::Separator();
        if (modal.hasFlowResult) {
            ImGui::TextWrapped("%s", modal.statusMessage);
            if (ImGui::Button("Close", ImVec2(160.0f, 0.0f))) {
                closeModal(modal, simClock);
            }
        } else {
            ImGui::Text("Members ready: %d (need %d)", playerOrganizationStore.crewMemberCount, MIN_CREW_MEMBERS_TO_FORM);
            ImGui::InputText("Crew name", modal.crewNameBuffer, sizeof(modal.crewNameBuffer));
            if (ImGui::Button("Establish crew", ImVec2(200.0f, 0.0f))) {
                if (tryFormalizeCrew(playerOrganizationStore, modal.crewNameBuffer, tickCount)) {
                    setModalStatus(modal, "Your crew is official on the street. Crew-tier crimes unlock.");
                    modal.hasFlowResult = true;
                } else {
                    setModalStatus(modal, "Could not formalize — need more members.");
                    modal.hasFlowResult = true;
                }
            }
        }
    } else if (modal.kind == GameModalKind::OrganizationCreation) {
        ImGui::Text("Organization incorporation");
        ImGui::Separator();
        const OrganizationFormLockReason lockReason = evaluateOrganizationFormLock(
            playerOrganizationStore, playerLawEnforcementStore, playerProfile, playerWallet);
        ImGui::TextDisabled("%s", organizationFormLockReasonToString(lockReason));
        if (modal.hasFlowResult) {
            ImGui::TextWrapped("%s", modal.statusMessage);
            if (ImGui::Button("Close", ImVec2(160.0f, 0.0f))) {
                closeModal(modal, simClock);
            }
        } else {
            ImGui::TextWrapped("You are graduating from a street gang to something with ledgers, fronts, and payroll.");
            ImGui::InputText("Organization name", modal.organizationNameBuffer, sizeof(modal.organizationNameBuffer));
            ImGui::InputText("Legitimate front", modal.organizationFrontBuffer, sizeof(modal.organizationFrontBuffer));
            ImGui::Text("Crew: %s | Members: %d", playerOrganizationStore.crewName, playerOrganizationStore.crewMemberCount);
            if (lockReason != OrganizationFormLockReason::None) {
                ImGui::BeginDisabled();
            }
            if (ImGui::Button("File incorporation", ImVec2(200.0f, 0.0f))) {
                if (tryFormalizeOrganization(
                        playerOrganizationStore,
                        modal.organizationNameBuffer,
                        modal.organizationFrontBuffer,
                        tickCount)) {
                    setModalStatus(modal, "Organization formed. Enterprise street crimes unlock.");
                    modal.hasFlowResult = true;
                } else {
                    setModalStatus(modal, "Requirements not met — check heat, network, and crime record.");
                    modal.hasFlowResult = true;
                }
            }
            if (lockReason != OrganizationFormLockReason::None) {
                ImGui::EndDisabled();
            }
        }
    }
    ImGui::End();
}

} // namespace Core
