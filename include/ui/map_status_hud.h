#pragma once

#include "character/player_profile.h"
#include "game/game_calendar.h"
#include "game/player_health.h"
#include "game/player_law_enforcement.h"
#include "game/player_law_intel.h"
#include "game/player_wallet.h"
#include "ui/viewport_state.h"

namespace Core {

void renderMapStatusHud(
    const PlayerWallet& playerWallet,
    const PlayerLawEnforcementStore& lawStore,
    const PlayerLawIntelStore& intelStore,
    const PlayerHealthStore& healthStore,
    const GameCalendarStore& calendarStore,
    const ViewportPickState& viewportPickState,
    float canvasPosX,
    float canvasPosY,
    float canvasWidth,
    float canvasHeight);

} // namespace Core
