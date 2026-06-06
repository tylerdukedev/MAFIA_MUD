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
#include "game/criminal_record.h"
#include "game/police_contacts.h"
#include "game/player_criminal_justice.h"
#include "game/evidence_system.h"
#include "game/investigation_case_store.h"
#include "game/player_law_enforcement.h"
#include "game/player_operations.h"
#include "game/player_wallet.h"
#include "game/player_organization.h"
#include "game/social_action_catalog.h"
#include "game/street_crime.h"
#include "game/player_world_state.h"
#include "game/game_calendar.h"
#include "game/player_health.h"
#include "game/player_law_intel.h"
#include "game/player_work_schedule.h"
#include "game/legal_counsel.h"
#include "game/bank_loan.h"
#include "game/property_listing_store.h"
#include "game/property_store.h"
#include "persistence/save_gameplay_stores.h"
#include "ui/game_modal_state.h"
#include "sim/character_agent.h"
#include "sim/agent_relationship_graph.h"
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
    PlayerOrganizationStore playerOrganizationStore{};
    PlayerStreetCrimeStore playerStreetCrimeStore{};
    PlayerSocialActionStore playerSocialActionStore{};
    PlayerLawEnforcementStore playerLawEnforcementStore{};
    InvestigationCaseStore investigationCaseStore{};
    EvidenceSystemStore evidenceSystemStore{};
    PlayerCriminalJusticeStore playerCriminalJusticeStore{};
    CriminalRecordStore playerCriminalRecordStore{};
    PoliceContactStore playerPoliceContactStore{};
    SaveGameplayStores gameplayStores{};
    CharacterAgentStore characterAgentStore{};
    AgentRelationshipGraph agentRelationshipGraph{};
    PropertyStore propertyStore{};
    PropertyListingStore propertyListingStore{};
    BankLoanStore bankLoanStore{};
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
