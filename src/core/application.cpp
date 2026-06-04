#include "core/application.h"
#include "game/player_world_state.h"
#include "world/region_table.h"
#include "ui/game_modal_state.h"
#include "ui/game_modal_ui.h"
#include "ui/game_ui.h"
#include "ui/dock_layout.h"
#include "procgen/world_generator.h"
#include "persistence/save_game.h"
#include "sim/sim_world_bindings.h"
#include "world/tile_vitality.h"
#include "world/landmark_table.h"
#include "game/player_wallet.h"
#include "persistence/save_gameplay_stores.h"
#include "persistence/playthrough_archive.h"
#include "utils/app_paths.h"
#include "world/city_control.h"
#include "sim/sim_event_queue.h"
#include "character/profile_builder.h"
#include "character/character_social_network.h"
#include "sim/world_event_store.h"
#if defined(CAPITALVICE_DEV_CONSOLE)
#include "dev/dev_console.h"
#endif
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#define GL_SILENCE_DEPRECATION
#if defined(__APPLE__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
#include <cstdio>

namespace Core {

namespace {
constexpr int32_t WINDOW_WIDTH = 1280;
constexpr int32_t WINDOW_HEIGHT = 800;
constexpr int32_t MIN_WINDOW_WIDTH = 960;
constexpr int32_t MIN_WINDOW_HEIGHT = 540;
constexpr const char* WINDOW_TITLE = "CapitalVice";
constexpr const char* GLSL_VERSION = "#version 130";
constexpr const char* IMGUI_INI_PATH = "capitalvice_layout.ini";
} // namespace

void Application::glfwErrorCallback(int errorCode, const char* description) {
    std::fprintf(stderr, "GLFW Error %d: %s\n", errorCode, description);
}

Application::Application()
    : window(nullptr)
    , imguiContext(nullptr)
    , chunkStore(worldConfig)
    , simClock(WorldConfig::DEFAULT_TICK_RATE_HZ)
    , worldSeed(DEFAULT_WORLD_SEED)
    , frontendScreen(FrontendScreen::MainMenu)
    , isWorldReady(false)
    , isRunning(false) {
    playerProfile = buildPlayerProfile(characterDraft);
}

Application::~Application() {
    shutdownImGui();
    shutdownWindow();
}

int Application::run() {
    initializeApplicationUserDataDirectory();
    glfwSetErrorCallback(glfwErrorCallback);
    if (!glfwInit()) {
        return 1;
    }
    if (!initializeWindow()) {
        glfwTerminate();
        return 1;
    }
    if (!initializeImGui()) {
        shutdownWindow();
        glfwTerminate();
        return 1;
    }
    isRunning = true;
    while (isRunning && !glfwWindowShouldClose(window)) {
        processFrame();
    }
    shutdownImGui();
    shutdownWindow();
    glfwTerminate();
    return 0;
}

bool Application::initializeWindow() {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, nullptr, nullptr);
    if (window == nullptr) {
        return false;
    }
    glfwSetWindowSizeLimits(window, MIN_WINDOW_WIDTH, MIN_WINDOW_HEIGHT, GLFW_DONT_CARE, GLFW_DONT_CARE);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    return true;
}

bool Application::initializeImGui() {
    IMGUI_CHECKVERSION();
    imguiContext = ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.IniFilename = IMGUI_INI_PATH;
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 4.0f;
    style.FrameRounding = 3.0f;
#if defined(CAPITALVICE_DEV_CONSOLE)
    style.WindowRounding = 0.0f;
    style.Colors[ImGuiCol_WindowBg].w = 1.0f;
#endif
    if (!ImGui_ImplGlfw_InitForOpenGL(window, true)) {
        return false;
    }
    if (!ImGui_ImplOpenGL3_Init(GLSL_VERSION)) {
        return false;
    }
#if defined(CAPITALVICE_DEV_CONSOLE)
    devConsoleLogAppend(devConsoleLog, "Dev console ready. Type help.");
#endif
    return true;
}

void Application::shutdownImGui() {
    if (imguiContext == nullptr) {
        return;
    }
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    imguiContext = nullptr;
}

void Application::shutdownWindow() {
    if (window != nullptr) {
        glfwDestroyWindow(window);
        window = nullptr;
    }
}

void Application::processFrame() {
    glfwPollEvents();
    if (frontendScreen == FrontendScreen::InGame && ImGui::IsKeyPressed(ImGuiKey_Space)) {
        simClock.togglePaused();
    }
    if (frontendScreen == FrontendScreen::InGame && ImGui::IsKeyPressed(ImGuiKey_S)) {
        simClock.stepOneTick();
    }
    updateSimulation();
    renderFrame();
}

void Application::updateSimulation() {
    if (frontendScreen != FrontendScreen::InGame || !isWorldReady || shouldPauseSimulationForModal(gameModalState)) {
        return;
    }
    const double deltaSeconds = ImGui::GetIO().DeltaTime;
    simClock.update(deltaSeconds);
    runSimulationTicks();
}

void Application::runSimulationTicks() {
    const int32_t ticksThisFrame = simClock.getTicksThisFrame();
    if (ticksThisFrame <= 0) {
        return;
    }
    const uint64_t tickCount = simClock.getTickCount();
    const uint64_t firstTick = tickCount - static_cast<uint64_t>(ticksThisFrame) + 1U;
    for (uint64_t tickIndex = firstTick; tickIndex <= tickCount; ++tickIndex) {
        systemRegistry.runTick(tickIndex);
    }
}

void Application::renderFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    updateContextHelpMode(contextHelpState);
    const bool hasSaveFile = saveFileExists(DEFAULT_SAVE_FILENAME);
    ApplicationMenuBarParams menuBarParams{};
    menuBarParams.screen = frontendScreen;
    menuBarParams.simClock = frontendScreen == FrontendScreen::InGame ? &simClock : nullptr;
    menuBarParams.hasSaveFile = hasSaveFile;
    menuBarParams.isWorldReady = isWorldReady;
    menuBarParams.helpManualState = &helpManualState;
    menuBarParams.panelVisibility = frontendScreen == FrontendScreen::InGame ? &panelVisibility : nullptr;
    ApplicationMenuBarEvents menuBarEvents = renderApplicationMenuBar(menuBarParams);
    if (menuBarEvents.requestedExitGame) {
        isRunning = false;
    }
    if (menuBarEvents.requestedLoadGame && frontendScreen == FrontendScreen::InGame) {
        loadSavedGame();
    }
    if (menuBarEvents.requestedSaveGame && frontendScreen == FrontendScreen::InGame) {
        saveCurrentGame();
    }
    renderHelpManualWindow(helpManualState);
    FrontendUiEvents frontendUiEvents = renderFrontendUi(frontendScreen, characterDraft, hasSaveFile, worldSeed);
    if (frontendUiEvents.requestedExitGame) {
        isRunning = false;
    }
    if (frontendUiEvents.requestedLoadGame) {
        if (loadSavedGame()) {
            frontendScreen = FrontendScreen::InGame;
        }
    }
    if (frontendUiEvents.requestedStartSimulation && !isWorldReady) {
        startNewSimulation();
    }
    if (frontendScreen == FrontendScreen::CharacterCreation || frontendScreen == FrontendScreen::CharacterFinalize) {
        playerProfile = buildPlayerProfile(characterDraft);
    }
    if (frontendScreen == FrontendScreen::InGame && isWorldReady) {
        if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S)) {
            saveCurrentGame();
        }
        renderGameUi(
            simClock,
            worldConfig,
            chunkStore,
            boroughVitalityStore,
            playerWallet,
            playerOperationsStore,
            playerOrganizationStore,
            playerStreetCrimeStore,
            playerLawEnforcementStore,
            playerCriminalJusticeStore,
            gameplayStores,
            characterAgentStore,
            worldEventStore,
            cityControlStore,
            simEventQueue,
            mapCrimeOverlayEnabled,
            panelVisibility,
            systemRegistry,
            mapCamera,
            viewportPickState,
            worldSeed,
            playerProfile,
            playerWorldState,
            gameModalState,
            contextHelpState);
    }
    renderContextHelpCursorOverlay(contextHelpState);
    renderContextHelpInspectPopup(contextHelpState, helpManualState);
#if defined(CAPITALVICE_DEV_CONSOLE)
    DevConsoleGameplaySnapshot devGameplaySnapshot{};
    devGameplaySnapshot.playerWallet = isWorldReady ? &playerWallet : nullptr;
    devGameplaySnapshot.cityControlStore = isWorldReady ? &cityControlStore : nullptr;
    devGameplaySnapshot.playerOperationsStore = isWorldReady ? &playerOperationsStore : nullptr;
    devGameplaySnapshot.playerOrganizationStore = isWorldReady ? &playerOrganizationStore : nullptr;
    devGameplaySnapshot.playerLawEnforcementStore = isWorldReady ? &playerLawEnforcementStore : nullptr;
    devGameplaySnapshot.playerCriminalJusticeStore = isWorldReady ? &playerCriminalJusticeStore : nullptr;
    devGameplaySnapshot.characterAgentStore = isWorldReady ? &characterAgentStore : nullptr;
    devGameplaySnapshot.worldEventStore = isWorldReady ? &worldEventStore : nullptr;
    devGameplaySnapshot.gameplayStores = isWorldReady ? &gameplayStores : nullptr;
    devGameplaySnapshot.tickCount = simClock.getTickCount();
    devGameplaySnapshot.isWorldReady = isWorldReady;
    if (ImGui::IsKeyPressed(ImGuiKey_F12, false)) {
        devConsoleToggleVisibility(devConsoleState);
    }
    devConsoleRender(devConsoleState, devConsoleLog, characterDraft, playerProfile, &devGameplaySnapshot);
#endif
    ImGui::Render();
    int framebufferWidth = 0;
    int framebufferHeight = 0;
    glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
    glViewport(0, 0, framebufferWidth, framebufferHeight);
    renderClearBackground();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window);
}

void Application::startNewSimulation() {
    playerProfile = buildPlayerProfile(characterDraft);
    resetBoroughVitalityStore(boroughVitalityStore);
    resetCityControlStore(cityControlStore);
    resetPlayerOperationsStore(playerOperationsStore);
    resetPlayerOrganizationStore(playerOrganizationStore);
    resetPlayerStreetCrimeStore(playerStreetCrimeStore);
    resetPlayerLawEnforcementStore(playerLawEnforcementStore);
    resetPlayerCriminalJusticeStore(playerCriminalJusticeStore);
    resetSaveGameplayStores(gameplayStores);
    playerWorldState = gameplayStores.worldState;
    resetGameModalState(gameModalState);
    resetWorldEventStore(worldEventStore);
    initializeCharacterAgentStore(characterAgentStore);
    spawnPersonalContactsFromDraft(characterDraft, characterAgentStore);
    clearSimEventQueue(simEventQueue);
    playerWallet = PlayerWallet{};
    playerWallet.cashCents = characterDraft.startingCashCents;
    WorldGenerator worldGenerator;
    worldGenerator.generate(worldConfig, chunkStore, worldSeed);
    rollupBoroughVitality(worldConfig, chunkStore, boroughVitalityStore);
    const SimWorldBindings simBindings{
        &chunkStore,
        &boroughVitalityStore,
        &worldConfig,
        &worldSeed,
        &playerWallet,
        &cityControlStore,
        &simEventQueue,
        &playerProfile,
        &playerOperationsStore,
        &playerOrganizationStore,
        &playerLawEnforcementStore,
        &playerCriminalJusticeStore,
        &playerStreetCrimeStore,
        &characterAgentStore,
        &worldEventStore};
    systemRegistry.initialize(
        simBindings,
        &characterAgentStore,
        &playerStreetCrimeStore,
        &playerLawEnforcementStore,
        &playerCriminalJusticeStore,
        &gameplayStores.calendarStore,
        &gameplayStores.workScheduleStore,
        &playerWorldState,
        &gameplayStores.playerHealthStore,
        &gameplayStores.populationHealthStore);
    requestDefaultGameDockLayout();
    panelVisibility = GamePanelVisibility{};
    mapCamera = MapCamera{};
    const LandmarkDefinition* startCity = getLandmarkDefinition(characterDraft.startingCityLandmarkIndex);
    if (startCity != nullptr) {
        mapCamera.centerOnTile(startCity->tileX, startCity->tileY, DEFAULT_MAP_PIXELS_PER_TILE);
        const RegionId regionId = regionIdFromBoroughPreferenceIndex(characterDraft.selectedBoroughIndex);
        initializePlayerWorldStateFromStart(playerWorldState, startCity->tileX, startCity->tileY, regionId);
    } else {
        initializeMapCameraForStartingBorough(mapCamera, characterDraft.selectedBoroughIndex);
    }
    gameplayStores.worldState = playerWorldState;
    viewportPickState = ViewportPickState{};
    simClock = SimClock(WorldConfig::DEFAULT_TICK_RATE_HZ);
    isWorldReady = true;
}

bool Application::saveCurrentGame() {
    if (!isWorldReady) {
        setSaveLoadStatusMessage("Save failed: no active game.");
        return false;
    }
    SaveGameSnapshot snapshot{};
    if (!buildSaveSnapshot(
            snapshot,
            worldSeed,
            characterDraft,
            simClock,
            mapCamera,
            chunkStore,
            boroughVitalityStore,
            playerWallet,
            cityControlStore,
            playerOperationsStore,
            worldEventStore,
            characterAgentStore,
            playerOrganizationStore,
            playerLawEnforcementStore,
            playerStreetCrimeStore,
            playerCriminalJusticeStore,
            gameplayStores,
            playerOperationsStore.workExperienceMonths)) {
        setSaveLoadStatusMessage("Save failed: could not capture world state.");
        return false;
    }
    snapshot.gameplayStores.worldState = playerWorldState;
    if (!saveGameToFile(DEFAULT_SAVE_FILENAME, snapshot)) {
        setSaveLoadStatusMessage("Save failed: could not write save file.");
        return false;
    }
    PlaythroughArchiveFile playthroughArchive{};
    loadPlaythroughArchiveFile(playthroughArchive);
    upsertCurrentPlaythroughSlot(
        playthroughArchive,
        characterDraft.nameBuffer,
        worldSeed,
        gameplayStores.calendarStore.totalDaysElapsed,
        playerWallet.cashCents,
        gameplayStores.narrativeArchiveStore);
    savePlaythroughArchiveFile(playthroughArchive);
    setSaveLoadStatusMessage("Game saved.");
    return true;
}

bool Application::loadSavedGame() {
    if (!saveFileExists(DEFAULT_SAVE_FILENAME)) {
        setSaveLoadStatusMessage("Load failed: no save file found.");
        return false;
    }
    SaveGameSnapshot snapshot{};
    if (!loadGameFromFile(DEFAULT_SAVE_FILENAME, snapshot)) {
        setSaveLoadStatusMessage("Load failed: save file is invalid or incompatible.");
        return false;
    }
    if (!applySaveSnapshot(
            snapshot,
            worldSeed,
            characterDraft,
            simClock,
            mapCamera,
            chunkStore,
            playerWallet,
            cityControlStore,
            playerOperationsStore,
            worldEventStore,
            characterAgentStore,
            playerOrganizationStore,
            playerLawEnforcementStore,
            playerStreetCrimeStore,
            playerCriminalJusticeStore,
            gameplayStores,
            playerOperationsStore.workExperienceMonths)) {
        setSaveLoadStatusMessage("Load failed: could not restore world state.");
        return false;
    }
    rollupBoroughVitality(worldConfig, chunkStore, boroughVitalityStore);
    const SimWorldBindings simBindings{
        &chunkStore,
        &boroughVitalityStore,
        &worldConfig,
        &worldSeed,
        &playerWallet,
        &cityControlStore,
        &simEventQueue,
        &playerProfile,
        &playerOperationsStore,
        &playerOrganizationStore,
        &playerLawEnforcementStore,
        &playerCriminalJusticeStore,
        &playerStreetCrimeStore,
        &characterAgentStore,
        &worldEventStore};
    systemRegistry.initialize(
        simBindings,
        &characterAgentStore,
        &playerStreetCrimeStore,
        &playerLawEnforcementStore,
        &playerCriminalJusticeStore,
        &gameplayStores.calendarStore,
        &gameplayStores.workScheduleStore,
        &playerWorldState,
        &gameplayStores.playerHealthStore,
        &gameplayStores.populationHealthStore);
    playerWorldState = gameplayStores.worldState;
    panelVisibility = GamePanelVisibility{};
    playerProfile = buildPlayerProfile(characterDraft);
    viewportPickState = ViewportPickState{};
    isWorldReady = true;
    setSaveLoadStatusMessage("Game loaded.");
    return true;
}

void Application::renderClearBackground() {
    glClearColor(0.08f, 0.09f, 0.11f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

} // namespace Core
