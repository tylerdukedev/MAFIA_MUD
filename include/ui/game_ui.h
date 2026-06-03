#pragma once

#include "character/character_draft.h"
#include "character/player_profile.h"
#include "core/sim_clock.h"
#include "game/player_law_enforcement.h"
#include "game/player_operations.h"
#include "game/player_wallet.h"
#include "game/player_organization.h"
#include "game/street_crime.h"
#include "sim/character_agent.h"
#include "ui/panel_visibility.h"
#include "sim/sim_event_queue.h"
#include "sim/world_event_store.h"
#include "sim/system_registry.h"
#include "world/city_control.h"
#include "game/player_world_state.h"
#include "ui/context_help.h"
#include "ui/game_modal_state.h"
#include "ui/help_manual.h"
#include "ui/map_camera.h"
#include "ui/viewport_state.h"
#include "world/chunk_store.h"
#include "world/tile_vitality.h"
#include "world/world_config.h"
#include <cstdint>

namespace Core {

enum class FrontendScreen : uint8_t {
    MainMenu = 0,
    CharacterCreation,
    CharacterFinalize,
    InGame
};

struct FrontendUiEvents {
    bool requestedNewGame = false;
    bool requestedLoadGame = false;
    bool requestedExitGame = false;
    bool requestedStartSimulation = false;
};

struct ApplicationMenuBarParams {
    FrontendScreen screen;
    SimClock* simClock;
    bool hasSaveFile;
    bool isWorldReady;
    HelpManualState* helpManualState;
    GamePanelVisibility* panelVisibility;
};

struct ApplicationMenuBarEvents {
    bool requestedSaveGame = false;
    bool requestedLoadGame = false;
    bool requestedExitGame = false;
};

void setSaveLoadStatusMessage(const char* message);
const char* getSaveLoadStatusMessage();

ApplicationMenuBarEvents renderApplicationMenuBar(const ApplicationMenuBarParams& params);

FrontendUiEvents renderFrontendUi(
    FrontendScreen& frontendScreen,
    CharacterDraft& characterDraft,
    bool hasSaveFile,
    uint64_t worldSeed);

void renderGameUi(
    SimClock& simClock,
    const WorldConfig& worldConfig,
    ChunkStore& chunkStore,
    const BoroughVitalityStore& boroughVitalityStore,
    PlayerWallet& playerWallet,
    PlayerOperationsStore& playerOperationsStore,
    PlayerOrganizationStore& playerOrganizationStore,
    PlayerStreetCrimeStore& playerStreetCrimeStore,
    PlayerLawEnforcementStore& playerLawEnforcementStore,
    CharacterAgentStore& characterAgentStore,
    const WorldEventStore& worldEventStore,
    CityControlStore& cityControlStore,
    SimEventQueue& simEventQueue,
    bool& mapCrimeOverlayEnabled,
    GamePanelVisibility& panelVisibility,
    SystemRegistry& systemRegistry,
    MapCamera& mapCamera,
    ViewportPickState& viewportPickState,
    uint64_t worldSeed,
    const PlayerProfile& playerProfile,
    PlayerWorldState& playerWorldState,
    GameModalState& gameModalState,
    ContextHelpState& contextHelpState);

} // namespace Core
