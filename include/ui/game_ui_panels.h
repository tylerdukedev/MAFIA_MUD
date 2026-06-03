#pragma once

#include "character/player_profile.h"
#include "game/player_operations.h"
#include "game/player_wallet.h"
#include "sim/character_agent.h"
#include "sim/sim_event_queue.h"
#include "sim/world_event_store.h"
#include "ui/context_help.h"
#include "ui/panel_visibility.h"
#include "ui/viewport_state.h"
#include "world/city_control.h"
#include "world/world_config.h"

namespace Core {

void renderOperationsPanel(
    PlayerOperationsStore& playerOperationsStore,
    PlayerWallet& playerWallet,
    CharacterAgentStore& characterAgentStore,
    const WorldEventStore& worldEventStore,
    SimEventQueue& simEventQueue,
    const PlayerProfile& playerProfile,
    uint64_t tickCount,
    GamePanelVisibility& panelVisibility,
    ContextHelpState& contextHelpState);

void renderBusinessPanel(
    const WorldConfig& worldConfig,
    PlayerOperationsStore& playerOperationsStore,
    PlayerWallet& playerWallet,
    SimEventQueue& simEventQueue,
    const PlayerProfile& playerProfile,
    const ViewportPickState& viewportPickState,
    GamePanelVisibility& panelVisibility,
    ContextHelpState& contextHelpState);

void renderContactsPanel(
    const CharacterAgentStore& characterAgentStore,
    GamePanelVisibility& panelVisibility,
    ContextHelpState& contextHelpState);

} // namespace Core
