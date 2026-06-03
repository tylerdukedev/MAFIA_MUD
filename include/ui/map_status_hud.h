#pragma once

#include "game/game_calendar.h"
#include "game/player_health.h"
#include "game/player_law_enforcement.h"
#include "game/player_law_intel.h"
#include "game/player_wallet.h"
#include "game/player_world_state.h"
#include "world/chunk_store.h"

namespace Core {

struct MapHudInteraction {
    bool requestFocusCharacterPanel = false;
    bool requestCenterOnPlayer = false;
};

void renderMapStatusHud(
    const PlayerWallet& playerWallet,
    const PlayerLawEnforcementStore& lawStore,
    const PlayerLawIntelStore& intelStore,
    const PlayerHealthStore& healthStore,
    const GameCalendarStore& calendarStore,
    const ChunkStore& chunkStore,
    const PlayerWorldState& worldState,
    MapHudInteraction& interaction,
    float canvasPosX,
    float canvasPosY,
    float canvasWidth,
    float canvasHeight);

} // namespace Core
