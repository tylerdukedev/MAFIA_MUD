#pragma once

#include "character/player_profile.h"
#include "game/player_criminal_justice.h"
#include "game/player_law_enforcement.h"
#include "game/player_law_intel.h"
#include "game/player_narrative_archive.h"
#include "game/legal_counsel.h"
#include "game/travel_modes.h"
#include "game/player_operations.h"
#include "game/player_wallet.h"
#include "game/street_crime.h"
#include "sim/character_agent.h"
#include "sim/sim_event_queue.h"
#include "sim/world_event_store.h"
#include "game/player_world_state.h"
#include "ui/context_help.h"
#include "ui/game_modal_state.h"
#include "ui/panel_visibility.h"
#include "core/sim_clock.h"
#include "ui/viewport_state.h"
#include "world/city_control.h"
#include "world/world_config.h"

namespace Core {

void renderOperationsPanel(
    PlayerOperationsStore& playerOperationsStore,
    PlayerOrganizationStore& playerOrganizationStore,
    PlayerStreetCrimeStore& playerStreetCrimeStore,
    PlayerLawEnforcementStore& playerLawEnforcementStore,
    PlayerLawIntelStore& playerLawIntelStore,
    PlayerCriminalJusticeStore& playerCriminalJusticeStore,
    PlayerLegalCounselStore& legalCounselStore,
    const PlayerNarrativeArchiveStore& narrativeArchiveStore,
    PlayerWallet& playerWallet,
    PlayerWorldState& playerWorldState,
    CharacterAgentStore& characterAgentStore,
    const WorldEventStore& worldEventStore,
    SimEventQueue& simEventQueue,
    const PlayerProfile& playerProfile,
    GameModalState& gameModalState,
    SimClock& simClock,
    uint64_t tickCount,
    GamePanelVisibility& panelVisibility,
    ContextHelpState& contextHelpState);

void renderBusinessPanel(
    const WorldConfig& worldConfig,
    PlayerOperationsStore& playerOperationsStore,
    PlayerWallet& playerWallet,
    SimEventQueue& simEventQueue,
    const PlayerProfile& playerProfile,
    const PlayerWorldState& playerWorldState,
    GameModalState& gameModalState,
    SimClock& simClock,
    const ViewportPickState& viewportPickState,
    uint64_t worldSeed,
    GamePanelVisibility& panelVisibility,
    ContextHelpState& contextHelpState);

void renderContactsPanel(
    const CharacterAgentStore& characterAgentStore,
    const PlayerOrganizationStore& playerOrganizationStore,
    const PlayerLawEnforcementStore& playerLawEnforcementStore,
    const PlayerLawIntelStore& playerLawIntelStore,
    GameModalState& gameModalState,
    SimClock& simClock,
    GamePanelVisibility& panelVisibility,
    ContextHelpState& contextHelpState);

} // namespace Core
