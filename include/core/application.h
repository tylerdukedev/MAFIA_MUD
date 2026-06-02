#pragma once

#include "character/character_draft.h"
#include "character/player_profile.h"
#include "core/sim_clock.h"
#include "sim/system_registry.h"
#include "sim/sim_context.h"
#include "ui/game_ui.h"
#include "ui/help_manual.h"
#include "ui/context_help.h"
#include "ui/map_camera.h"
#include "ui/viewport_state.h"
#include "world/chunk_store.h"
#include "world/district_store.h"
#include "world/tile_field_store.h"
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
    DistrictStore districtStore;
    TileFieldStore tileFieldStore;
    SimContext simContext;
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
    void initializeWorldLayers();
    bool saveCurrentGame();
    bool loadSavedGame();
};

} // namespace Core
