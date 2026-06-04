#include "ui/game_modal_ui.h"
#include "character/character_social_network.h"
#include "game/landlord_contact.h"
#include "game/player_employment.h"
#include "game/job_catalog.h"
#include "game/crime_legal_tier.h"
#include "game/criminal_record.h"
#include "game/police_contacts.h"
#include "game/player_criminal_justice.h"
#include "game/player_law_enforcement.h"
#include "game/player_wallet.h"
#include "game/player_organization.h"
#include "game/player_organization_ui.h"
#include "game/operation_types.h"
#include "game/player_operations.h"
#include "game/player_work_schedule.h"
#include "game/covert_action_executor.h"
#include "game/player_information_feed.h"
#include "game/player_narrative_archive.h"
#include "game/action_reason_catalog.h"
#include "game/agent_relation_events.h"
#include "sim/character_agent.h"
#include "world/business_node_table.h"
#include "imgui.h"
#include <cstring>

namespace Core {

namespace {

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

void beginJobInterviewModal(GameModalState& modal, int32_t businessNodeIndex, SimClock& simClock, uint64_t worldSeed) {
    openModal(modal, simClock, GameModalKind::JobInterview, "Job interview in progress.");
    modal.businessNodeIndex = businessNodeIndex;
    buildJobInterviewSession(modal.interviewSession, businessNodeIndex, worldSeed);
    modal.interviewQuestionIndex = 0;
    modal.interviewScore = 0;
}

void beginJobResignationModal(GameModalState& modal, int32_t resigningFromBusinessIndex, int32_t acceptingAtBusinessIndex, SimClock& simClock) {
    openModal(modal, simClock, GameModalKind::JobResignation, "Leaving your current job requires notice or immediate resignation.");
    modal.resigningFromBusinessIndex = resigningFromBusinessIndex;
    modal.acceptingAtBusinessIndex = acceptingAtBusinessIndex;
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

void beginBondHearingModal(GameModalState& modal, SimClock& simClock) {
    openModal(modal, simClock, GameModalKind::BondHearing, "Arraignment — post bond or wait for court in lockup.");
}

void beginCourtHearingModal(GameModalState& modal, SimClock& simClock) {
    openModal(modal, simClock, GameModalKind::CourtHearing, "Court is in session — the judge will rule on your case.");
}

void beginInformationFeedModal(GameModalState& modal, int32_t feedItemIndex, SimClock& simClock) {
    openModal(modal, simClock, GameModalKind::InformationFeed, "Story update — simulation paused.");
    modal.informationFeedIndex = feedItemIndex;
}

void beginCovertActionModal(
    GameModalState& modal,
    CovertActionKind actionKind,
    int32_t targetAgentIndex,
    const CharacterAgentStore& agentStore,
    const PlayerOrganizationStore& organizationStore,
    const PlayerLawEnforcementStore& lawStore,
    const PlayerLawIntelStore& intelStore,
    SimClock& simClock) {
    char statusBuffer[96];
    std::snprintf(statusBuffer, sizeof(statusBuffer), "%s — choose a reason.", covertActionKindToLabel(actionKind));
    openModal(modal, simClock, GameModalKind::CovertAction, statusBuffer);
    modal.covertActionKind = actionKind;
    modal.targetAgentIndex = targetAgentIndex;
    modal.selectedCovertReasonIndex = -1;
    modal.selectedCovertReasonId = ActionReasonId::None;
    modal.useCrewProxyForCovertAction = false;
    modal.covertReasonCount = collectCovertActionReasons(
        actionKind,
        targetAgentIndex,
        agentStore,
        organizationStore,
        lawStore,
        intelStore,
        modal.covertReasonOffers,
        MAX_ACTION_REASON_OFFERS);
}

void tickCriminalJusticeModals(
    GameModalState& modal,
    PlayerCriminalJusticeStore& justiceStore,
    SimClock& simClock) {
    if (modal.isActive) {
        return;
    }
    const CustodyPhase phase = getPlayerCustodyPhase(justiceStore);
    if (phase == CustodyPhase::Arrested) {
        beginBondHearingModal(modal, simClock);
        return;
    }
    if (phase == CustodyPhase::InJail) {
        beginBondHearingModal(modal, simClock);
        return;
    }
    if (phase == CustodyPhase::AwaitingCourt && isPlayerCourtModalPending(justiceStore)) {
        beginCourtHearingModal(modal, simClock);
    }
}

void tickWorkScheduleModals(
    GameModalState& modal,
    const PlayerWorkScheduleStore& workScheduleStore,
    const GameCalendarStore& calendarStore,
    SimClock& simClock) {
    if (modal.isActive || !shouldOpenWorkCommuteModal(workScheduleStore, calendarStore)) {
        return;
    }
    beginWorkDayCommuteModal(modal, computeWorkDayLateness(workScheduleStore, calendarStore), simClock);
}

void renderGameModalOverlay(
    GameModalState& modal,
    SimClock& simClock,
    PlayerOperationsStore& playerOperationsStore,
    PlayerOrganizationStore& playerOrganizationStore,
    PlayerLawEnforcementStore& playerLawEnforcementStore,
    PlayerCriminalJusticeStore& playerCriminalJusticeStore,
    CriminalRecordStore& criminalRecordStore,
    PoliceContactStore& policeContactStore,
    PlayerLegalCounselStore& legalCounselStore,
    PlayerHealthStore& playerHealthStore,
    PlayerLawIntelStore& lawIntelStore,
    PlayerNarrativeArchiveStore& narrativeArchiveStore,
    PlayerInformationFeedStore& informationFeedStore,
    PlayerWallet& playerWallet,
    PlayerWorldState& playerWorldState,
    const ChunkStore& chunkStore,
    const WorldConfig& worldConfig,
    PlayerWorkScheduleStore& workScheduleStore,
    GameCalendarStore& calendarStore,
    CharacterAgentStore& characterAgentStore,
    SimEventQueue& simEventQueue,
    const PlayerProfile& playerProfile,
    uint64_t tickCount,
    uint64_t worldSeed) {
    (void)legalCounselStore;
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
    ImGui::SetNextWindowFocus();
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
        const JobInterviewSession& session = modal.interviewSession;
        if (modal.interviewQuestionIndex >= session.questionCount) {
            if (!modal.hasFlowResult) {
                const bool passed = evaluateJobInterviewPass(session, modal.businessNodeIndex);
                const bool hired = passed && tryHirePlayerAtBusiness(
                    playerOperationsStore,
                    playerProfile,
                    characterAgentStore,
                    modal.businessNodeIndex,
                    session.totalScore);
                if (!passed) {
                    recordJobRejection(playerOperationsStore, modal.businessNodeIndex, tickCount);
                }
                if (hired) {
                    const JobDefinitionExtension* extension = getJobDefinitionExtension(modal.businessNodeIndex);
                    if (extension != nullptr
                        && (extension->perkFlags & static_cast<uint32_t>(JobPerkFlags::HealthBenefits)) != 0U) {
                        playerHealthStore.hasHealthCoverage = true;
                    }
                }
                setModalStatus(modal, hired ? "You are hired. Wages accrue while employed." : "They passed on you this time.");
                modal.hasFlowResult = true;
            }
            ImGui::TextWrapped("%s", modal.statusMessage);
            if (ImGui::Button("Close", ImVec2(160.0f, 0.0f))) {
                closeModal(modal, simClock);
            }
        } else {
            const JobInterviewQuestionInstance& question = session.questions[modal.interviewQuestionIndex];
            ImGui::Text("Question %d of %d", modal.interviewQuestionIndex + 1, session.questionCount);
            ImGui::TextWrapped("%s", question.prompt);
            ImGui::Spacing();
            for (int32_t answerIndex = 0; answerIndex < JOB_INTERVIEW_ANSWER_COUNT; ++answerIndex) {
                if (ImGui::Selectable(question.answers[answerIndex].text, modal.selectedAnswerIndex == answerIndex)) {
                    modal.selectedAnswerIndex = answerIndex;
                }
            }
            if (modal.selectedAnswerIndex >= 0 && ImGui::Button("Submit answer", ImVec2(180.0f, 0.0f))) {
                modal.interviewSession.totalScore += question.answers[modal.selectedAnswerIndex].scoreDelta;
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
        char dateLabel[64];
        formatCalendarDateLabel(calendarStore, dateLabel, sizeof(dateLabel));
        ImGui::TextDisabled("%s | Shift %d:00-%d:00", dateLabel, workScheduleStore.shiftStartHour, workScheduleStore.shiftEndHour);
        ImGui::Separator();
        ImGui::TextWrapped("%s", modal.statusMessage);
        if (ImGui::Button("Go to work (on time)", ImVec2(200.0f, 0.0f))) {
            markWorkShiftStarted(workScheduleStore, playerWorldState, calendarStore, false);
            closeModal(modal, simClock);
        }
        ImGui::SameLine();
        if (ImGui::Button("Go to work (late)", ImVec2(200.0f, 0.0f))) {
            markWorkShiftStarted(workScheduleStore, playerWorldState, calendarStore, true);
            setModalStatus(modal, "You clocked in late. Expect reduced pay until travel rules expand.");
            closeModal(modal, simClock);
        }
        if (ImGui::Button("Call out / skip shift", ImVec2(200.0f, 0.0f))) {
            markWorkShiftSkipped(workScheduleStore, calendarStore);
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
            playerOrganizationStore, playerLawEnforcementStore, playerCriminalJusticeStore, playerProfile, playerWallet);
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
    } else if (modal.kind == GameModalKind::BondHearing) {
        const CriminalRecordStore& record = criminalRecordStore;
        const CriminalCharge* latestCharge = getLatestPendingCharge(record);

        // Header — jurisdiction
        if (latestCharge != nullptr && latestCharge->jurisdictionLabel[0] != '\0') {
            ImGui::TextDisabled("%s", latestCharge->jurisdictionLabel);
        } else {
            ImGui::TextDisabled("Criminal Court of the City of New York");
        }
        ImGui::Separator();
        ImGui::Text("ARRAIGNMENT — THE PEOPLE vs. DEFENDANT");
        ImGui::Spacing();

        // Charge block
        if (latestCharge != nullptr) {
            ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "COUNT 1: %s", chargeTypeToString(latestCharge->chargeType));
            ImGui::TextDisabled("  Statute: %s", chargeTypeToStatuteLabel(latestCharge->chargeType));
            if (latestCharge->officerName[0] != '\0') {
                // Find rank from police contacts for display
                const int32_t contactIdx = findPoliceContactByName(policeContactStore, latestCharge->officerName);
                const char* rankLabel = "Officer";
                if (contactIdx >= 0) {
                    const PoliceContactState* officer = getPoliceContact(policeContactStore, contactIdx);
                    if (officer != nullptr) {
                        rankLabel = policeRankToString(officer->rank);
                    }
                }
                ImGui::TextDisabled("  Arresting: %s %s", rankLabel, latestCharge->officerName);
            }
        } else {
            ImGui::TextWrapped("%s", playerCriminalJusticeStore.lastCustodyLabel);
        }

        // Prior record summary
        ImGui::Spacing();
        const int32_t priorArrests = playerCriminalJusticeStore.arrestCount - 1;
        if (priorArrests > 0 || record.convictionCount > 0) {
            ImGui::TextDisabled("Prior record: %d arrest(s), %d conviction(s), %d prior felony(ies)",
                priorArrests,
                record.convictionCount,
                record.priorFelonyCount);
        } else {
            ImGui::TextDisabled("Prior record: None");
        }

        // Bond info
        ImGui::Separator();
        ImGui::Text("Status: %s", custodyPhaseToString(getPlayerCustodyPhase(playerCriminalJusticeStore)));
        char bondBuffer[32];
        formatCashCents(bondBuffer, sizeof(bondBuffer), playerCriminalJusticeStore.bondCents);
        ImGui::Text("Bail set: %s cash or bond", bondBuffer);
        ImGui::TextDisabled("Rivals may move on your territory while you are inside.");
        ImGui::Spacing();

        if (ImGui::Button("Post bail", ImVec2(160.0f, 0.0f))) {
            if (tryPayPlayerBond(
                    playerCriminalJusticeStore,
                    playerWallet,
                    playerLawEnforcementStore,
                    playerOrganizationStore,
                    playerWorldState,
                    chunkStore,
                    worldConfig,
                    tickCount)) {
                setModalStatus(modal, playerCriminalJusticeStore.lastCustodyLabel);
                closeModal(modal, simClock);
            } else {
                setModalStatus(modal, "Not enough cash for bail.");
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Remain in custody", ImVec2(160.0f, 0.0f))) {
            commitPlayerCustodyDetention(playerCriminalJusticeStore, tickCount);
            setModalStatus(modal, playerCriminalJusticeStore.lastCustodyLabel);
            closeModal(modal, simClock);
        }
    } else if (modal.kind == GameModalKind::CovertAction) {
        const char* displayName = nullptr;
        const char* roleLabel = nullptr;
        tryGetAgentDisplayLabels(characterAgentStore, modal.targetAgentIndex, displayName, roleLabel);
        ImGui::Text("%s", covertActionKindToLabel(modal.covertActionKind));
        if (displayName != nullptr) {
            ImGui::TextDisabled("Target: %s (%s)", displayName, roleLabel);
        }
        ImGui::Separator();
        if (modal.hasFlowResult) {
            ImGui::TextWrapped("%s", modal.statusMessage);
            if (ImGui::Button("Close", ImVec2(160.0f, 0.0f))) {
                closeModal(modal, simClock);
            }
        } else if (modal.covertReasonCount <= 0) {
            ImGui::TextWrapped("No valid reasons for this target right now. Build intel or change the relationship first.");
            if (ImGui::Button("Close", ImVec2(160.0f, 0.0f))) {
                closeModal(modal, simClock);
            }
        } else {
            ImGui::TextWrapped("Pick why you are doing this. The reason is logged for headlines and your archive.");
            for (int32_t reasonIndex = 0; reasonIndex < modal.covertReasonCount; ++reasonIndex) {
                const ActionReasonOffer& offer = modal.covertReasonOffers[reasonIndex];
                if (ImGui::Selectable(offer.label, modal.selectedCovertReasonIndex == reasonIndex)) {
                    modal.selectedCovertReasonIndex = reasonIndex;
                    modal.selectedCovertReasonId = offer.reasonId;
                }
                if (modal.selectedCovertReasonIndex == reasonIndex) {
                    ImGui::TextDisabled("%s", offer.description);
                }
            }
            if (modal.covertActionKind == CovertActionKind::AssassinateTarget && playerOrganizationStore.crewMemberCount > 0) {
                ImGui::Checkbox("Use crew proxy (more deniable, still risky)", &modal.useCrewProxyForCovertAction);
            }
            if (modal.selectedCovertReasonIndex >= 0 && ImGui::Button("Commit action", ImVec2(200.0f, 0.0f))) {
                const CovertActionResult actionResult = executeCovertActionWithReason(
                    modal.covertActionKind,
                    modal.selectedCovertReasonId,
                    modal.targetAgentIndex,
                    modal.useCrewProxyForCovertAction,
                    lawIntelStore,
                    playerLawEnforcementStore,
                    characterAgentStore,
                    playerWallet,
                    playerOrganizationStore,
                    narrativeArchiveStore,
                    calendarStore,
                    worldSeed,
                    tickCount);
                const int32_t beatCount = getNarrativeBeatCount(narrativeArchiveStore);
                if (beatCount > 0) {
                    const NarrativeBeatRecord& beat = narrativeArchiveStore.beats[beatCount - 1];
                    pushInformationFeedItem(
                        informationFeedStore,
                        InformationChannel::Newspaper,
                        beat.headline,
                        beat.narrativeTag,
                        tickCount,
                        true);
                }
                if (actionResult.succeeded) {
                    setModalStatus(modal, "Done. The city will remember this — check your narrative archive.");
                } else {
                    setModalStatus(modal, "Failed or partial. Heat and relationships shifted.");
                }
                modal.hasFlowResult = true;
            }
        }
    } else if (modal.kind == GameModalKind::CourtHearing) {
        ImGui::Text("Court");
        ImGui::Separator();
        const CrimeLegalTier tier = static_cast<CrimeLegalTier>(playerCriminalJusticeStore.pendingLegalTier);
        ImGui::Text("Charge tier: %s", crimeLegalTierToString(tier));
        const LawyerTierDefinition* counsel = getLawyerTierDefinition(legalCounselStore.hiredLawyerTier);
        ImGui::Text("Counsel: %s", counsel->displayName);
        if (modal.hasFlowResult) {
            ImGui::TextWrapped("%s", modal.statusMessage);
            if (ImGui::Button("Close", ImVec2(160.0f, 0.0f))) {
                closeModal(modal, simClock);
            }
        } else {
            const CustodyPhase custodyPhase = getPlayerCustodyPhase(playerCriminalJusticeStore);
            if (custodyPhase == CustodyPhase::OnBail) {
                if (ImGui::Button("Attend court", ImVec2(200.0f, 0.0f))) {
                    resolvePlayerCourt(playerCriminalJusticeStore, playerLawEnforcementStore, legalCounselStore, characterAgentStore, worldSeed, tickCount);
                    const CourtOutcome outcome = static_cast<CourtOutcome>(playerCriminalJusticeStore.lastCourtOutcome);
                    char outcomeBuffer[96];
                    std::snprintf(
                        outcomeBuffer,
                        sizeof(outcomeBuffer),
                        "Ruling: %s — %s",
                        courtOutcomeToString(outcome),
                        playerCriminalJusticeStore.lastCustodyLabel);
                    setModalStatus(modal, outcomeBuffer);
                    modal.hasFlowResult = true;
                }
                ImGui::SameLine();
                if (ImGui::Button("Skip court date", ImVec2(200.0f, 0.0f))) {
                    trySkipPlayerCourtDate(playerCriminalJusticeStore, playerLawEnforcementStore, tickCount);
                    setModalStatus(modal, playerCriminalJusticeStore.lastCustodyLabel);
                    modal.hasFlowResult = true;
                }
            } else if (ImGui::Button("Enter courtroom", ImVec2(200.0f, 0.0f))) {
                resolvePlayerCourt(playerCriminalJusticeStore, playerLawEnforcementStore, legalCounselStore, characterAgentStore, worldSeed, tickCount);
                const CourtOutcome outcome = static_cast<CourtOutcome>(playerCriminalJusticeStore.lastCourtOutcome);
                char outcomeBuffer[96];
                std::snprintf(
                    outcomeBuffer,
                    sizeof(outcomeBuffer),
                    "Ruling: %s — %s",
                    courtOutcomeToString(outcome),
                    playerCriminalJusticeStore.lastCustodyLabel);
                setModalStatus(modal, outcomeBuffer);
                modal.hasFlowResult = true;
            }
        }
    } else if (modal.kind == GameModalKind::InformationFeed) {
        ImGui::Text("Story update");
        ImGui::Separator();
        if (modal.informationFeedIndex >= 0 && modal.informationFeedIndex < informationFeedStore.itemCount) {
            const InformationFeedItem& feedItem = informationFeedStore.items[modal.informationFeedIndex];
            const char* channelLabel = feedItem.channel == InformationChannel::Newspaper
                ? "Newspaper"
                : (feedItem.channel == InformationChannel::Intel ? "Intel" : "Rumor");
            ImGui::Text("[%s] %s", channelLabel, feedItem.headline);
            ImGui::TextWrapped("%s", feedItem.body);
        } else {
            ImGui::TextWrapped("This alert is no longer available.");
        }
        if (ImGui::Button("Close", ImVec2(160.0f, 0.0f))) {
            closeModal(modal, simClock);
        }
    } else if (modal.kind == GameModalKind::JobResignation) {
        ImGui::Text("Job Resignation");
        const BusinessNodeDefinition* currentJob = getBusinessNodeDefinition(modal.resigningFromBusinessIndex);
        const BusinessNodeDefinition* newJob = getBusinessNodeDefinition(modal.acceptingAtBusinessIndex);
        if (currentJob != nullptr) {
            ImGui::TextDisabled("Leaving: %s", currentJob->fullName);
        }
        if (newJob != nullptr) {
            ImGui::TextDisabled("Accepting: %s", newJob->fullName);
        }
        ImGui::Separator();
        if (modal.hasFlowResult) {
            ImGui::TextWrapped("%s", modal.statusMessage);
            if (ImGui::Button("Close", ImVec2(160.0f, 0.0f))) {
                closeModal(modal, simClock);
            }
        } else {
            ImGui::TextWrapped("You must resign from your current job before accepting the new position. How do you want to handle this?");
            ImGui::Spacing();
            if (ImGui::Button("Give two-week notice", ImVec2(220.0f, 0.0f))) {
                const int32_t bossSlot = BOSS_AGENT_SLOT_INDEX;
                if (characterAgentStore.states[bossSlot].isActive) {
                    adjustAgentOpinion(characterAgentStore, bossSlot, 8);
                    characterAgentStore.states[bossSlot].trust = std::min(100, characterAgentStore.states[bossSlot].trust + 5);
                }
                for (int i = 0; i < 2; ++i) {
                    if (playerOperationsStore.employedBusinessIndices[i] == modal.resigningFromBusinessIndex) {
                        playerOperationsStore.employedBusinessIndices[i] = -1;
                        break;
                    }
                }
                setModalStatus(modal, "Your boss appreciates the notice. The job change is complete.");
                modal.hasFlowResult = true;
            }
            ImGui::TextDisabled("Boss opinion +8, trust +5");
            ImGui::Spacing();
            if (ImGui::Button("Quit immediately", ImVec2(220.0f, 0.0f))) {
                const int32_t bossSlot = BOSS_AGENT_SLOT_INDEX;
                if (characterAgentStore.states[bossSlot].isActive) {
                    adjustAgentOpinion(characterAgentStore, bossSlot, -12);
                    characterAgentStore.states[bossSlot].trust = std::max(0, characterAgentStore.states[bossSlot].trust - 8);
                }
                for (int i = 0; i < 2; ++i) {
                    if (playerOperationsStore.employedBusinessIndices[i] == modal.resigningFromBusinessIndex) {
                        playerOperationsStore.employedBusinessIndices[i] = -1;
                        break;
                    }
                }
                setModalStatus(modal, "You walked out. Your boss is not happy about it.");
                modal.hasFlowResult = true;
            }
            ImGui::TextDisabled("Boss opinion -12, trust -8");
        }
    }
    ImGui::End();
}

} // namespace Core
