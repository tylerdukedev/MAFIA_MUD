#include "ui/game_modal_state.h"

namespace Core {

void resetGameModalState(GameModalState& state) {
    state = GameModalState{};
}

bool shouldBlockGameplayPanels(const GameModalState& state) {
    return state.isActive && state.lockOtherPanels;
}

bool shouldPauseSimulationForModal(const GameModalState& state) {
    return state.isActive && state.pauseSimulation;
}

} // namespace Core
