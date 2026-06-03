#pragma once

#include "game/job_catalog.h"
#include "game/action_reason_catalog.h"
#include "game/player_law_intel.h"
#include <cstdint>

namespace Core {

enum class GameModalKind : uint8_t {
    None = 0,
    JobInterview = 1,
    ApartmentApplication = 2,
    WorkDayCommute = 3,
    CrewRecruitment = 4,
    CrewFormalize = 5,
    OrganizationCreation = 6,
    BondHearing = 7,
    CourtHearing = 8,
    CovertAction = 9,
};

struct GameModalState {
    bool isActive = false;
    GameModalKind kind = GameModalKind::None;
    bool pauseSimulation = false;
    bool lockOtherPanels = false;
    int32_t businessNodeIndex = -1;
    int32_t targetAgentIndex = -1;
    JobInterviewSession interviewSession{};
    int32_t interviewQuestionIndex = 0;
    int32_t interviewScore = 0;
    int32_t selectedAnswerIndex = -1;
    bool hasFlowResult = false;
    bool isLateForWork = false;
    char statusMessage[128]{};
    char crewNameBuffer[32]{};
    char organizationNameBuffer[48]{};
    char organizationFrontBuffer[48]{};
    CovertActionKind covertActionKind = CovertActionKind::BribePolice;
    ActionReasonId selectedCovertReasonId = ActionReasonId::None;
    ActionReasonOffer covertReasonOffers[MAX_ACTION_REASON_OFFERS]{};
    int32_t covertReasonCount = 0;
    int32_t selectedCovertReasonIndex = -1;
    bool useCrewProxyForCovertAction = false;
};

void resetGameModalState(GameModalState& state);
bool shouldBlockGameplayPanels(const GameModalState& state);
bool shouldPauseSimulationForModal(const GameModalState& state);

} // namespace Core
