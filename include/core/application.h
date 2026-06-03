#pragma once

#include "character/character_draft.h"
#include "character/player_profile.h"
#include "core/sim_clock.h"
#include "sim/system_registry.h"
#include "ui/game_ui.h"
#include "ui/help_manual.h"
#include "ui/context_help.h"
#include "ui/map_camera.h"
#include "ui/viewport_state.h"
#include "game/player_operations.h"
#include "game/player_wallet.h"
#include "game/player_world_state.h"
#include "ui/game_modal_state.h"
#include "sim/character_agent.h"
#include "sim/sim_event_queue.h"
#include "sim/world_event_store.h"
#include "ui/panel_visibility.h"
#include "world/chunk_store.h"
#include "world/city_control.h"
#include "world/tile_vitality.h"
#include "world/world_config.h"
#include <GLFW/glfw3.h>
#include <cstdint>

struct ImGuiContext;

#if defined(CAPITALVICE_DEV_CONSOLE)
#include "dev/dev_console.h"
#endif

namespace Core {

class Application {
public:
    Application();
    ~Application();
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    int run();

private:
    GLFWwindow* window;
    ImGuiContext* imguiContext;
    WorldConfig worldConfig;
    ChunkStore chunkStore;
    BoroughVitalityStore boroughVitalityStore{};
    PlayerWallet playerWallet{};
    PlayerWorldState playerWorldState{};
    GameModalState gameModalState{};
    PlayerOperationsStore playerOperationsStore{};
    CharacterAgentStore characterAgentStore{};
    CityControlStore cityControlStore{};
    GamePanelVisibility panelVisibility{};
    SimEventQueue simEventQueue{};
    WorldEventStore worldEventStore{};
    bool mapCrimeOverlayEnabled = false;
    SimClock simClock;
    SystemRegistry systemRegistry;
    MapCamera mapCamera;
    ViewportPickState viewportPickState;
    CharacterDraft characterDraft;
    PlayerProfile playerProfile;
    HelpManualState helpManualState;
    ContextHelpState contextHelpState;
    uint64_t worldSeed;
    FrontendScreen frontendScreen;
    bool isWorldReady;
    bool isRunning;
#if defined(CAPITALVICE_DEV_CONSOLE)
    DevConsoleState devConsoleState;
    DevConsoleLog devConsoleLog;
#endif
    static void glfwErrorCallback(int errorCode, const char* description);
    bool initializeWindow();
    bool initializeImGui();
    void shutdownImGui();
    void shutdownWindow();
    void processFrame();
    void updateSimulation();
    void runSimulationTicks();
    void renderFrame();
    void renderClearBackground();
    void startNewSimulation();
    bool saveCurrentGame();
    bool loadSavedGame();
};

} // namespace Core
