#pragma once

#include <cstdint>

namespace Core {

enum class GameModalKind : uint8_t {
    None = 0,
    JobInterview = 1,
    ApartmentApplication = 2,
    WorkDayCommute = 3,
};

struct GameModalState {
    bool isActive = false;
    GameModalKind kind = GameModalKind::None;
    bool pauseSimulation = false;
    bool lockOtherPanels = false;
    int32_t businessNodeIndex = -1;
    int32_t interviewQuestionIndex = 0;
    int32_t interviewScore = 0;
    int32_t selectedAnswerIndex = -1;
    bool isLateForWork = false;
    char statusMessage[128]{};
};

void resetGameModalState(GameModalState& state);
bool shouldBlockGameplayPanels(const GameModalState& state);
bool shouldPauseSimulationForModal(const GameModalState& state);

} // namespace Core
