#pragma once

#include "ui/game_modal_state.h"
#include "ui/game_ui_frame_context.h"

namespace Core {

void renderOperationsPanel(const GameUiFrameContext& frame, GameModalState& gameModalState);
void renderBusinessPanel(const GameUiFrameContext& frame, GameModalState& gameModalState);
void renderContactsPanel(const GameUiFrameContext& frame, GameModalState& gameModalState);

} // namespace Core
