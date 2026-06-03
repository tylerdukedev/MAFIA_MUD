#include "ui/map_renderer.h"
#include "ui/game_ui.h"
#include "ui/help_manual.h"
#include "ui/context_help.h"
#include "character/character_start.h"
#include "character/character_tables.h"
#include "character/profile_builder.h"
#include "game/economy_constants.h"
#include "game/player_wallet.h"
#include "sim/sim_event_queue.h"
#include "world/city_control.h"
#include <cstdio>
#include <cfloat>
#include <algorithm>
#include <cstring>
#include "ui/dock_layout.h"
#include "ui/landmark_renderer.h"
#include "world/landmark_table.h"
#include "world/region_table.h"
#include "world/tile_vitality.h"
#include "sim/borough_vitality_system.h"
#include "imgui.h"

namespace Core {

char saveLoadStatusMessage[128]{};

namespace {
constexpr float MIN_WINDOW_WIDTH = 960.0f;
constexpr float MIN_WINDOW_HEIGHT = 540.0f;
constexpr float SPEED_OPTIONS[] = {0.25f, 0.5f, 1.0f, 2.0f, 4.0f};
constexpr int32_t SPEED_OPTION_COUNT = 5;
constexpr float ZOOM_WHEEL_FACTOR = 1.12f;

void renderCharacterCreationPreviewPanel(const CharacterDraft& characterDraft) {
    ImGui::BeginChild("CharacterPreviewPanel", ImVec2(0.0f, 0.0f), true);
    ImGui::Text("Character Preview");
    ImGui::Separator();
    char descriptionBuffer[256];
    buildCharacterDescription(descriptionBuffer, sizeof(descriptionBuffer), characterDraft);
    ImGui::TextWrapped("%s", descriptionBuffer);
    ImGui::Spacing();
    ImGui::Separator();
    char cashBuffer[32];
    formatCashCents(cashBuffer, sizeof(cashBuffer), characterDraft.startingCashCents);
    ImGui::Text("Starting cash: %s", cashBuffer);
    const LandmarkDefinition* startingCity = getLandmarkDefinition(characterDraft.startingCityLandmarkIndex);
    if (startingCity != nullptr) {
        ImGui::Text("Starting city: %s", startingCity->fullName);
        ImGui::TextDisabled("Map label: %s", startingCity->mapLabel);
    } else {
        ImGui::TextDisabled("Starting city: roll borough to place");
    }
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextWrapped("%s", getGenerationRoleSummary(characterDraft.generationId).data());
    ImGui::EndChild();
}

void renderCharacterCreationForm(CharacterDraft& characterDraft, FrontendUiEvents& frontendUiEvents, FrontendScreen& frontendScreen) {
    ImGui::BeginChild("CharacterCreationForm", ImVec2(0.0f, 0.0f), false);
    ImGui::Text("Create your character");
    ImGui::Separator();
    ImGui::InputText("Name", characterDraft.nameBuffer, sizeof(characterDraft.nameBuffer));
    int32_t nationalityIndex = static_cast<int32_t>(characterDraft.nationalityId) - 1;
    int32_t heritageIndex = static_cast<int32_t>(characterDraft.heritageId) - 1;
    int32_t generationIndex = static_cast<int32_t>(characterDraft.generationId) - 1;
    int32_t backgroundIndex = static_cast<int32_t>(characterDraft.backgroundId) - 1;
    if (ImGui::Combo(
            "Nationality",
            &nationalityIndex,
            [](void*, int index) { return getNationalityLabel(index); },
            nullptr,
            getNationalityCount())) {
        characterDraft.nationalityId = nationalityIdFromIndex(nationalityIndex);
    }
    if (ImGui::Combo(
            "Heritage",
            &heritageIndex,
            [](void*, int index) { return getHeritageLabel(index); },
            nullptr,
            getHeritageCount())) {
        characterDraft.heritageId = heritageIdFromIndex(heritageIndex);
    }
    if (ImGui::Combo(
            "Generation",
            &generationIndex,
            [](void*, int index) { return getGenerationLabel(index); },
            nullptr,
            getGenerationCount())) {
        characterDraft.generationId = generationIdFromIndex(generationIndex);
    }
    ImGui::SliderInt("Age", &characterDraft.age, CHARACTER_CREATION_MIN_AGE, CHARACTER_CREATION_MAX_AGE);
    if (ImGui::Combo(
            "Background",
            &backgroundIndex,
            [](void*, int index) { return getBackgroundLabel(index); },
            nullptr,
            getBackgroundCount())) {
        characterDraft.backgroundId = backgroundIdFromIndex(backgroundIndex);
    }
    ImGui::Combo(
        "Starting Borough",
        &characterDraft.selectedBoroughIndex,
        [](void*, int index) { return getBoroughPreferenceLabel(index); },
        nullptr,
        getBoroughPreferenceCount());
    ImGui::Spacing();
    ImGui::Separator();
    if (ImGui::Button("Start New Game", ImVec2(220.0f, 0.0f))) {
        frontendUiEvents.requestedStartSimulation = true;
        frontendScreen = FrontendScreen::InGame;
    }
    ImGui::SameLine();
    if (ImGui::Button("Back", ImVec2(120.0f, 0.0f))) {
        frontendScreen = FrontendScreen::MainMenu;
    }
    ImGui::EndChild();
}

void renderMainMenuScreen(FrontendUiEvents& frontendUiEvents, FrontendScreen& frontendScreen, bool hasSaveFile) {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    const ImVec2 panelSize(560.0f, 380.0f);
    const ImVec2 panelPos(
        viewport->WorkPos.x + (viewport->WorkSize.x - panelSize.x) * 0.5f,
        viewport->WorkPos.y + (viewport->WorkSize.y - panelSize.y) * 0.5f);
    ImGui::SetNextWindowPos(panelPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(panelSize, ImGuiCond_Always);
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    if (!ImGui::Begin("Main Menu", nullptr, windowFlags)) {
        ImGui::End();
        return;
    }
    ImGui::Dummy(ImVec2(0.0f, 8.0f));
    ImGui::SetWindowFontScale(2.0f);
    ImGui::Text("CAPITAL VICE");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::Separator();
    ImGui::Spacing();
    if (ImGui::Button("New Game", ImVec2(-1.0f, 0.0f))) {
        frontendUiEvents.requestedNewGame = true;
        frontendScreen = FrontendScreen::CharacterCreation;
    }
    if (!hasSaveFile) {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button("Load Game", ImVec2(-1.0f, 0.0f))) {
        frontendUiEvents.requestedLoadGame = true;
    }
    if (!hasSaveFile) {
        ImGui::EndDisabled();
        ImGui::TextDisabled("No save file found.");
    }
    if (ImGui::Button("Options", ImVec2(-1.0f, 0.0f))) {
    }
    if (ImGui::Button("Exit Game", ImVec2(-1.0f, 0.0f))) {
        frontendUiEvents.requestedExitGame = true;
    }
    ImGui::End();
}

void renderCharacterCreationScreen(
    CharacterDraft& characterDraft,
    FrontendUiEvents& frontendUiEvents,
    FrontendScreen& frontendScreen,
    uint64_t worldSeed) {
    initializeCharacterDraftDefaults(characterDraft);
    static int32_t lastRolledBoroughIndex = -1;
    if (characterDraft.selectedBoroughIndex != lastRolledBoroughIndex) {
        rollCharacterStartPlacement(characterDraft, worldSeed);
        lastRolledBoroughIndex = characterDraft.selectedBoroughIndex;
    }
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    const ImVec2 panelSize(960.0f, 520.0f);
    const ImVec2 panelPos(
        viewport->WorkPos.x + (viewport->WorkSize.x - panelSize.x) * 0.5f,
        viewport->WorkPos.y + (viewport->WorkSize.y - panelSize.y) * 0.5f);
    ImGui::SetNextWindowPos(panelPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(panelSize, ImGuiCond_Always);
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    if (!ImGui::Begin("Character Creation", nullptr, windowFlags)) {
        ImGui::End();
        return;
    }
    ImGui::Columns(2, "CharacterCreationColumns", true);
    ImGui::SetColumnWidth(0, panelSize.x * 0.48f);
    renderCharacterCreationForm(characterDraft, frontendUiEvents, frontendScreen);
    ImGui::NextColumn();
    renderCharacterCreationPreviewPanel(characterDraft);
    ImGui::Columns(1);
    ImGui::End();
}

const char* getTerrainName(TerrainId terrainId) {
    switch (terrainId) {
    case TerrainId::Water: return "Water";
    case TerrainId::Road: return "Road";
    case TerrainId::Building: return "Building";
    case TerrainId::Park: return "Park";
    case TerrainId::Plaza: return "Plaza";
    case TerrainId::OpenLand: return "Land";
    default: return "None";
    }
}

} // namespace

ApplicationMenuBarEvents renderApplicationMenuBar(const ApplicationMenuBarParams& params) {
    ApplicationMenuBarEvents menuBarEvents{};
    if (!ImGui::BeginMainMenuBar()) {
        return menuBarEvents;
    }
    const bool isInGame = params.screen == FrontendScreen::InGame;
    if (isInGame && params.simClock != nullptr) {
        if (ImGui::BeginMenu("File")) {
            if (!params.isWorldReady) {
                ImGui::BeginDisabled();
            }
            if (ImGui::MenuItem("Save Game", "Ctrl+S")) {
                menuBarEvents.requestedSaveGame = true;
            }
            if (!params.isWorldReady) {
                ImGui::EndDisabled();
            }
            if (!params.hasSaveFile) {
                ImGui::BeginDisabled();
            }
            if (ImGui::MenuItem("Load Game")) {
                menuBarEvents.requestedLoadGame = true;
            }
            if (!params.hasSaveFile) {
                ImGui::EndDisabled();
            }
            if (ImGui::MenuItem("Exit Game")) {
                menuBarEvents.requestedExitGame = true;
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Simulation")) {
            if (ImGui::MenuItem(params.simClock->isPaused() ? "Resume" : "Pause", "Space")) {
                params.simClock->togglePaused();
            }
            if (ImGui::MenuItem("Step One Tick", "S")) {
                params.simClock->stepOneTick();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            if (ImGui::MenuItem("Reset Panel Layout")) {
                resetDockLayout();
            }
            ImGui::EndMenu();
        }
    }
    if (ImGui::BeginMenu("Help")) {
        if (params.helpManualState != nullptr) {
            const bool manualOpen = params.helpManualState->isOpen;
            if (ImGui::MenuItem("Manual", nullptr, manualOpen)) {
                params.helpManualState->isOpen = !manualOpen;
            }
        }
        ImGui::MenuItem("Inspect Mode", "Hold Ctrl", false, false);
        ImGui::Separator();
        ImGui::BeginDisabled();
        ImGui::MenuItem("About Capital Vice");
        ImGui::EndDisabled();
        ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
    return menuBarEvents;
}

namespace {

void renderSimulationPanel(
    SimClock& simClock,
    const WorldConfig& worldConfig,
    const ChunkStore& chunkStore,
    const BoroughVitalityStore& boroughVitalityStore,
    bool& mapCrimeOverlayEnabled,
    const SystemRegistry& systemRegistry,
    uint64_t worldSeed,
    ContextHelpState& contextHelpState) {
    ImGui::SetNextWindowSizeConstraints(ImVec2(240.0f, 200.0f), ImVec2(FLT_MAX, FLT_MAX));
    if (ImGui::Begin("Simulation")) {
        contextHelpPanelTag(
            "Simulation Panel",
            "Clock status, speed controls, and world summary.",
            "sim_clock",
            contextHelpState);
        if (saveLoadStatusMessage[0] != '\0') {
            ImGui::TextWrapped("%s", saveLoadStatusMessage);
            ImGui::Separator();
        }
        ImGui::Text("Phase 6 - Economy & Cities");
        ImGui::Text("World seed: %llu", static_cast<unsigned long long>(worldSeed));
        ImGui::Separator();
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        ImGui::Text("Window: %.0f x %.0f", ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y);
        char simStatusLine[32];
        std::snprintf(simStatusLine, sizeof(simStatusLine), "Sim: %s", simClock.isPaused() ? "PAUSED" : "RUNNING");
        contextHelpTextLine(
            simStatusLine,
            "Pause with Space or the Simulation menu. While paused, S steps one tick.",
            "sim_clock",
            contextHelpState);
        char tickRateLine[32];
        std::snprintf(tickRateLine, sizeof(tickRateLine), "Tick rate: %.0f Hz", simClock.getTickRateHz());
        contextHelpTextLine(tickRateLine, "Fixed simulation rate. Multiple ticks may run per frame at high speed.", "sim_clock", contextHelpState);
        ImGui::Text("Speed: %.2fx", simClock.getSpeedMultiplier());
        ImGui::Text("Tick count: %llu", static_cast<unsigned long long>(simClock.getTickCount()));
        ImGui::Text("Ticks this frame: %d", simClock.getTicksThisFrame());
        ImGui::Separator();
        ImGui::Text("Speed multiplier");
        if (contextHelpState.isInspectMode) {
            ImGui::BeginDisabled();
        }
        for (int32_t index = 0; index < SPEED_OPTION_COUNT; ++index) {
            const float speedOption = SPEED_OPTIONS[index];
            char labelBuffer[16];
            std::snprintf(labelBuffer, sizeof(labelBuffer), "%.2gx", speedOption);
            if (ImGui::RadioButton(labelBuffer, simClock.getSpeedMultiplier() == static_cast<double>(speedOption))) {
                simClock.setSpeedMultiplier(static_cast<double>(speedOption));
            }
            if (index + 1 < SPEED_OPTION_COUNT) {
                ImGui::SameLine();
            }
        }
        if (contextHelpState.isInspectMode) {
            ImGui::EndDisabled();
        }
        ImGui::Separator();
        contextHelpTextLine(
            "World tile grid",
            "512x512 world stored in ChunkStore. See World Data in the Manual.",
            "world_data",
            contextHelpState);
        ImGui::Text("World: %d x %d tiles", worldConfig.WORLD_WIDTH_TILES, worldConfig.WORLD_HEIGHT_TILES);
        ImGui::Text("Chunk size: %d x %d", worldConfig.CHUNK_SIZE, worldConfig.CHUNK_SIZE);
        ImGui::Text("Chunks: %d x %d (%d total)", worldConfig.CHUNK_COUNT_X, worldConfig.CHUNK_COUNT_Y, chunkStore.getTotalChunkCount());
        ImGui::Text("Active chunks: %d", chunkStore.getActiveChunkCount());
        ImGui::Text("Boroughs: %d", RegionTable::getPlayableRegionCount());
        ImGui::Separator();
        char systemsLine[32];
        std::snprintf(systemsLine, sizeof(systemsLine), "Systems: %d", systemRegistry.getSystemCount());
        contextHelpTextLine(systemsLine, "Registered simulation systems called each tick.", "system_registry", contextHelpState);
        for (int32_t index = 0; index < systemRegistry.getSystemCount(); ++index) {
            const ISimSystem* system = systemRegistry.getSystem(index);
            if (system != nullptr) {
                ImGui::BulletText("%s (tick %llu)", system->getName(), static_cast<unsigned long long>(system->getLastTickCount()));
            }
        }
        const BoroughVitalitySystem* vitalitySystem = systemRegistry.getBoroughVitalitySystem();
        if (vitalitySystem != nullptr) {
            ImGui::Text(
                "Last borough rollup tick: %llu",
                static_cast<unsigned long long>(vitalitySystem->getLastRollupTickCount()));
        }
        ImGui::Separator();
        contextHelpSectionHeader(
            "Borough Vitality",
            "Aggregated economic health and pressure metrics per borough.",
            "borough_vitality_overview",
            contextHelpState);
        for (int32_t regionIndex = 1; regionIndex < static_cast<int32_t>(RegionId::COUNT); ++regionIndex) {
            const RegionId regionId = static_cast<RegionId>(regionIndex);
            const BoroughVitalitySnapshot* snapshot = getBoroughSnapshot(boroughVitalityStore, regionId);
            if (snapshot == nullptr) {
                continue;
            }
            char boroughLine[128];
            std::snprintf(
                boroughLine,
                sizeof(boroughLine),
                "%s: health %.0f | pop %u | crime %.0f%% | unemp %.0f%%",
                RegionTable::getRegionShortName(regionId).data(),
                snapshot->economicHealth,
                snapshot->totalPopulation,
                snapshot->crimeRate * 100.0f,
                snapshot->unemploymentRate * 100.0f);
            contextHelpTextLine(boroughLine, "Rollup runs every 40 simulation ticks.", "borough_health_formula", contextHelpState);
        }
        ImGui::Separator();
        contextHelpSectionHeader(
            "Map Display",
            "Optional heat tint on the map viewport.",
            "map_viewport",
            contextHelpState);
        ImGui::Checkbox("Crime heat overlay", &mapCrimeOverlayEnabled);
        ImGui::Separator();
        ImGui::TextDisabled("Space = pause/resume | S = step tick | Ctrl = inspect help");
    }
    ImGui::End();
}

int64_t computeClaimCostCentsForProfile(const PlayerProfile& playerProfile) {
    if (playerProfile.draft.backgroundId == BackgroundId::StreetHustler) {
        return STREET_HUSTLER_CLAIM_COST_CENTS;
    }
    return DEFAULT_CLAIM_CITY_COST_CENTS;
}

void renderCharacterPanel(const PlayerProfile& playerProfile, const PlayerWallet& playerWallet, ContextHelpState& contextHelpState) {
    ImGui::SetNextWindowSizeConstraints(ImVec2(320.0f, 400.0f), ImVec2(FLT_MAX, FLT_MAX));
    if (ImGui::Begin("Character")) {
        contextHelpPanelTag(
            "Character Panel",
            "Your identity and foundational trait profile during play.",
            "character_panel",
            contextHelpState);
        char identityBuffer[96];
        buildIdentityLabel(identityBuffer, sizeof(identityBuffer), playerProfile.draft);
        char descriptionBuffer[256];
        buildCharacterDescription(descriptionBuffer, sizeof(descriptionBuffer), playerProfile.draft);
        contextHelpSectionHeader(
            "Identity",
            "Core choices from character creation.",
            "profile_overview",
            contextHelpState);
        char nameLine[48];
        std::snprintf(nameLine, sizeof(nameLine), "Name: %s", playerProfile.draft.nameBuffer);
        contextHelpTextLine(nameLine, "Display name shown in descriptions and reports.", "char_name", contextHelpState);
        char ageLine[24];
        std::snprintf(ageLine, sizeof(ageLine), "Age: %d", playerProfile.draft.age);
        contextHelpTextLine(ageLine, "Age nudges street vs institutional opportunity paths.", "profile_builder", contextHelpState);
        char nationalityLine[64];
        std::snprintf(
            nationalityLine,
            sizeof(nationalityLine),
            "Nationality: %.*s",
            static_cast<int>(getNationalityName(playerProfile.draft.nationalityId).size()),
            getNationalityName(playerProfile.draft.nationalityId).data());
        contextHelpTextLine(nationalityLine, "Citizenship and passport context for legitimacy and language.", "profile_builder", contextHelpState);
        char heritageLine[64];
        std::snprintf(
            heritageLine,
            sizeof(heritageLine),
            "Heritage: %.*s",
            static_cast<int>(getHeritageName(playerProfile.draft.heritageId).size()),
            getHeritageName(playerProfile.draft.heritageId).data());
        contextHelpTextLine(heritageLine, "Ethnic heritage nudges network and cultural stats.", "profile_builder", contextHelpState);
        char generationLine[64];
        std::snprintf(
            generationLine,
            sizeof(generationLine),
            "Generation: %.*s",
            static_cast<int>(getGenerationName(playerProfile.draft.generationId).size()),
            getGenerationName(playerProfile.draft.generationId).data());
        contextHelpTextLine(generationLine, "Generation sets the base tradeoff for all trait axes.", "char_generation", contextHelpState);
        char backgroundLine[64];
        std::snprintf(
            backgroundLine,
            sizeof(backgroundLine),
            "Background: %.*s",
            static_cast<int>(getBackgroundName(playerProfile.draft.backgroundId).size()),
            getBackgroundName(playerProfile.draft.backgroundId).data());
        contextHelpTextLine(backgroundLine, "Starting career path before the story begins.", "profile_builder", contextHelpState);
        char boroughLine[64];
        std::snprintf(
            boroughLine,
            sizeof(boroughLine),
            "Borough: %.*s",
            static_cast<int>(getBoroughPreferenceName(playerProfile.draft.selectedBoroughIndex).size()),
            getBoroughPreferenceName(playerProfile.draft.selectedBoroughIndex).data());
        contextHelpTextLine(boroughLine, "Preferred starting borough. Gameplay territory links come later.", "profile_builder", contextHelpState);
        const LandmarkDefinition* startingCity = getLandmarkDefinition(playerProfile.draft.startingCityLandmarkIndex);
        if (startingCity != nullptr) {
            char cityLine[96];
            std::snprintf(cityLine, sizeof(cityLine), "Starting city: %s", startingCity->fullName);
            contextHelpTextLine(cityLine, "Random landmark node in your starting borough.", "city_panel", contextHelpState);
        }
        contextHelpSectionHeader(
            "Money",
            "Cash in cents. Legit and crime income accrue each tick.",
            "economy_overview",
            contextHelpState);
        char cashLine[48];
        formatCashCents(cashLine, sizeof(cashLine), playerWallet.cashCents);
        contextHelpTextLine(cashLine, "Spendable balance for city claims and future actions.", "economy_overview", contextHelpState);
        ImGui::Text("Legit income: %.2f c/tick", playerWallet.legitIncomePerTickCents);
        ImGui::Text("Crime income: %.2f c/tick", playerWallet.crimeIncomePerTickCents);
        if (playerWallet.lastDeltaKind != WalletDeltaKind::None) {
            char deltaBuffer[48];
            formatCashCents(deltaBuffer, sizeof(deltaBuffer), playerWallet.lastDeltaCents < 0 ? -playerWallet.lastDeltaCents : playerWallet.lastDeltaCents);
            const char* deltaLabel = playerWallet.lastDeltaKind == WalletDeltaKind::Loss ? "Last change"
                : (playerWallet.lastDeltaKind == WalletDeltaKind::GainLegit ? "Last legit gain" : "Last crime gain");
            ImGui::Text("%s: %s%s", deltaLabel, playerWallet.lastDeltaCents < 0 ? "-" : "+", deltaBuffer);
        }
        if (isWalletBroke(playerWallet)) {
            ImGui::TextWrapped("You are broke. Scrape together cash or claim a city when you can afford it.");
        }
        ImGui::TextDisabled("Aspiration: build income and connections; family vs legit forks come later.");
        char identityLine[128];
        std::snprintf(identityLine, sizeof(identityLine), "Identity label: %s", identityBuffer);
        contextHelpTextLine(identityLine, "Short identity string built from generation and heritage.", "profile_overview", contextHelpState);
        contextHelpSectionHeader("Description", "Narrative summary from your draft.", "char_creation", contextHelpState);
        contextHelpWrappedText(descriptionBuffer, "Character description", "Full prose summary of your character.", "char_creation", contextHelpState);
        contextHelpSectionHeader(
            "Foundational Traits",
            "Trait axes are one profile layer among future systems.",
            "trait_axes",
            contextHelpState);
        ImGui::TextDisabled("Hover for tooltips. Hold Ctrl and click for Manual links.");
        if (ImGui::CollapsingHeader("Network Access", ImGuiTreeNodeFlags_DefaultOpen)) {
            contextHelpStatBar(
                "Ethnic Network",
                playerProfile.networkAccess.ethnicNetwork,
                "Enclave contacts and introductions.",
                "stat_ethnic_network",
                contextHelpState);
            contextHelpStatBar(
                "Political Machine",
                playerProfile.networkAccess.politicalMachine,
                "Ward bosses and patronage access.",
                "stat_political_machine",
                contextHelpState);
            contextHelpStatBar(
                "Law Enforcement Channel",
                playerProfile.networkAccess.lawEnforcementChannel,
                "Informal police and clerk influence.",
                "stat_law_channel",
                contextHelpState);
            contextHelpStatBar(
                "Business Association",
                playerProfile.networkAccess.businessAssociation,
                "Trade groups and supplier ties.",
                "stat_business_assoc",
                contextHelpState);
            contextHelpStatBar(
                "Import Pipeline",
                playerProfile.networkAccess.importPipeline,
                "Smuggling and freight routes.",
                "stat_import_pipeline",
                contextHelpState);
        }
        if (ImGui::CollapsingHeader("Legitimacy", ImGuiTreeNodeFlags_DefaultOpen)) {
            contextHelpStatBar(
                "Police Attention Decay",
                playerProfile.legitimacy.policeAttentionDecay,
                "How fast heat cools when quiet.",
                "stat_police_decay",
                contextHelpState);
            contextHelpStatBar(
                "Shell Company Ease",
                playerProfile.legitimacy.shellCompanyEase,
                "Speed of standing up fronts.",
                "stat_shell_company",
                contextHelpState);
            contextHelpStatBar(
                "Public Job Access",
                playerProfile.legitimacy.publicFacingJobAccess,
                "Respectable cover employment.",
                "stat_public_job",
                contextHelpState);
            contextHelpStatBar(
                "Mainstream Suspicion",
                playerProfile.legitimacy.mainstreamSuspicion,
                "Baseline scrutiny from society.",
                "stat_mainstream_suspicion",
                contextHelpState);
        }
        if (ImGui::CollapsingHeader("Loyalty Bias", ImGuiTreeNodeFlags_DefaultOpen)) {
            contextHelpStatBar(
                "Faction Resistance",
                playerProfile.loyaltyBias.ethnicFactionResistance,
                "Pushback against rival factions.",
                "stat_faction_resist",
                contextHelpState);
            contextHelpStatBar(
                "Kin Alliance",
                playerProfile.loyaltyBias.kinAlliancePreference,
                "Family-first loyalty tendency.",
                "stat_kin_alliance",
                contextHelpState);
            contextHelpStatBar(
                "Detection Risk",
                playerProfile.loyaltyBias.mainstreamDetectionRisk,
                "Risk mainstream detects organized ties.",
                "stat_detection_risk",
                contextHelpState);
            contextHelpStatBar(
                "Individualistic Loyalty",
                playerProfile.loyaltyBias.individualisticLoyalty,
                "Personal gain over group duty.",
                "stat_individual_loyalty",
                contextHelpState);
        }
        if (ImGui::CollapsingHeader("Cultural Competency", ImGuiTreeNodeFlags_DefaultOpen)) {
            contextHelpStatBar(
                "In-Group Negotiation",
                playerProfile.culturalCompetency.inGroupNegotiation,
                "Deals inside your heritage group.",
                "stat_in_group_neg",
                contextHelpState);
            contextHelpStatBar(
                "Out-Group Negotiation",
                playerProfile.culturalCompetency.outGroupNegotiation,
                "Deals with outsiders and officials.",
                "stat_out_group_neg",
                contextHelpState);
            contextHelpStatBar(
                "Cross-Ethnic Penalty",
                playerProfile.culturalCompetency.crossEthnicPenalty,
                "Friction working across ethnic lines.",
                "stat_cross_ethnic",
                contextHelpState);
            contextHelpStatBar(
                "Language Access",
                playerProfile.culturalCompetency.languageAccess,
                "Mainstream language proficiency.",
                "stat_language",
                contextHelpState);
            contextHelpStatBar(
                "Translate Bonus",
                playerProfile.culturalCompetency.translateBonus,
                "Edge when using intermediaries.",
                "stat_translate",
                contextHelpState);
        }
        if (ImGui::CollapsingHeader("Opportunity Paths", ImGuiTreeNodeFlags_DefaultOpen)) {
            contextHelpStatBar(
                "Street Crime Path",
                playerProfile.opportunityPaths.streetCrimePath,
                "Street-level criminal aptitude.",
                "stat_street_crime",
                contextHelpState);
            contextHelpStatBar(
                "Organizer Path",
                playerProfile.opportunityPaths.organizerPath,
                "Crew and neighborhood organizing.",
                "stat_organizer",
                contextHelpState);
            contextHelpStatBar(
                "Institutional Path",
                playerProfile.opportunityPaths.institutionalPath,
                "Unions, offices, institutional graft.",
                "stat_institutional",
                contextHelpState);
            contextHelpStatBar(
                "Corporate Path",
                playerProfile.opportunityPaths.corporatePath,
                "Finance and corporate infiltration.",
                "stat_corporate",
                contextHelpState);
        }
    }
    ImGui::End();
}

void renderBoroughsPanel(const BoroughVitalityStore& boroughVitalityStore, ContextHelpState& contextHelpState) {
    ImGui::SetNextWindowSizeConstraints(ImVec2(220.0f, 160.0f), ImVec2(FLT_MAX, FLT_MAX));
    if (ImGui::Begin("Boroughs")) {
        contextHelpPanelTag(
            "Boroughs Panel",
            "Playable boroughs with live vitality bars.",
            "borough_vitality_overview",
            contextHelpState);
        for (int32_t regionIndex = 1; regionIndex < static_cast<int32_t>(RegionId::COUNT); ++regionIndex) {
            const RegionId regionId = static_cast<RegionId>(regionIndex);
            const BoroughVitalitySnapshot* snapshot = getBoroughSnapshot(boroughVitalityStore, regionId);
            if (snapshot == nullptr) {
                continue;
            }
            ImGui::PushID(regionIndex);
            ImGui::Text("%s", RegionTable::getRegionName(regionId).data());
            contextHelpStatBar(
                "Economic Health",
                snapshot->economicHealth / BOROUGH_HEALTH_MAX,
                "Weighted rollup of business, crime, law, and influence pressures.",
                "borough_health_formula",
                contextHelpState);
            ImGui::Text("Population: %u", snapshot->totalPopulation);
            contextHelpStatBar(
                "Crime Rate",
                snapshot->crimeRate,
                "Higher crime drags health and pushes migration out.",
                "crime_law_opposition",
                contextHelpState);
            contextHelpStatBar(
                "Unemployment",
                snapshot->unemploymentRate,
                "Derived from low business vitality rollup.",
                "population_model",
                contextHelpState);
            ImGui::Separator();
            ImGui::PopID();
        }
    }
    ImGui::End();
}

void renderCityPanel(
    const WorldConfig& worldConfig,
    const ChunkStore& chunkStore,
    const BoroughVitalityStore& boroughVitalityStore,
    const CityControlStore& cityControlStore,
    const PlayerWallet& playerWallet,
    SimEventQueue& simEventQueue,
    const PlayerProfile& playerProfile,
    const ViewportPickState& viewportPickState,
    ContextHelpState& contextHelpState) {
    ImGui::SetNextWindowSizeConstraints(ImVec2(300.0f, 220.0f), ImVec2(FLT_MAX, FLT_MAX));
    if (ImGui::Begin("City")) {
        contextHelpPanelTag("City Panel", "Stats for a selected map landmark city node.", "city_panel", contextHelpState);
        if (!viewportPickState.hasLandmarkSelection || viewportPickState.selectedLandmarkIndex < 0) {
            ImGui::TextDisabled("Click a labeled city landmark on the map.");
            ImGui::TextWrapped("Cities are high-value control nodes. They run hotter and are harder to take than ordinary tiles.");
        } else {
            const int32_t landmarkIndex = viewportPickState.selectedLandmarkIndex;
            const LandmarkDefinition* landmark = getLandmarkDefinition(landmarkIndex);
            if (landmark == nullptr) {
                ImGui::TextDisabled("Invalid city selection.");
            } else {
                const WorldCoord coord{landmark->tileX, landmark->tileY};
                contextHelpTextLine(
                    landmark->fullName,
                    "Full city name. Map labels may use a short form (e.g. LGA).",
                    "landmarks_overview",
                    contextHelpState);
                ImGui::Text("Map label: %s", landmark->mapLabel);
                if (worldConfig.isWithinWorldBounds(coord)) {
                    const RegionId regionId = chunkStore.getRegionAt(coord);
                    ImGui::Text("Borough: %s", RegionTable::getRegionName(regionId).data());
                } else {
                    ImGui::Text("Borough: out of bounds");
                }
                ImGui::Text("Tile: (%d, %d)", landmark->tileX, landmark->tileY);
                const uint8_t tileCrime = chunkStore.getCrimePressureAt(coord);
                const uint8_t tilePlayer = chunkStore.getPlayerInfluenceAt(coord);
                const float liveHeat = pressureUint8ToNormalized(tileCrime);
                const float controlDifficulty = std::min(
                    1.0f,
                    LANDMARK_CONTROL_DIFFICULTY * (0.65f + liveHeat * 0.35f) - pressureUint8ToNormalized(tilePlayer) * 0.15f);
                const RegionId boroughId = chunkStore.getRegionAt(coord);
                const BoroughVitalitySnapshot* boroughSnapshot = getBoroughSnapshot(boroughVitalityStore, boroughId);
                ImGui::Separator();
                contextHelpSectionHeader(
                    "City Vitality",
                    "Live tile pressures and borough rollup context.",
                    "city_hot_nodes",
                    contextHelpState);
                contextHelpStatBar(
                    "Heat Index",
                    liveHeat,
                    "Crime pressure at the landmark tile; seeded hotter than surroundings.",
                    "city_hot_nodes",
                    contextHelpState);
                contextHelpStatBar(
                    "Control Difficulty",
                    controlDifficulty,
                    "Base landmark difficulty adjusted by live crime and player influence.",
                    "landmark_control",
                    contextHelpState);
                ImGui::Text("Borough influence weight: %.1fx", LANDMARK_BOROUGH_INFLUENCE_WEIGHT);
                ImGui::Text("Economic weight: %u", chunkStore.getEconomicWeightAt(coord));
                if (boroughSnapshot != nullptr) {
                    ImGui::Text("Borough health: %.0f", boroughSnapshot->economicHealth);
                    ImGui::Text("Borough crime: %.0f%%", boroughSnapshot->crimeRate * 100.0f);
                }
                const bool isPlayerOwned = getCityOwnerId(cityControlStore, landmarkIndex) == PLAYER_OWNER_ID;
                if (isPlayerOwned) {
                    ImGui::Text("Control status: Your operation");
                } else if (isCityClaimed(cityControlStore, landmarkIndex)) {
                    ImGui::Text("Control status: Contested");
                } else {
                    ImGui::Text("Control status: Unclaimed");
                    const int64_t claimCostCents = computeClaimCostCentsForProfile(playerProfile);
                    char claimCostBuffer[32];
                    formatCashCents(claimCostBuffer, sizeof(claimCostBuffer), claimCostCents);
                    if (!canAffordCash(playerWallet, claimCostCents)) {
                        ImGui::BeginDisabled();
                    }
                    if (ImGui::Button("Establish operation")) {
                        pushSimEvent(simEventQueue, SimEventType::ClaimCity, landmarkIndex);
                    }
                    if (!canAffordCash(playerWallet, claimCostCents)) {
                        ImGui::EndDisabled();
                    }
                    ImGui::SameLine();
                    ImGui::Text("(%s)", claimCostBuffer);
                }
            }
        }
    }
    ImGui::End();
}

void renderTileInspectorPanel(
    const WorldConfig& worldConfig,
    const ChunkStore& chunkStore,
    const BoroughVitalityStore& boroughVitalityStore,
    const ViewportPickState& viewportPickState,
    ContextHelpState& contextHelpState) {
    ImGui::SetNextWindowSizeConstraints(ImVec2(320.0f, 140.0f), ImVec2(FLT_MAX, FLT_MAX));
    if (ImGui::Begin("Tile Inspector")) {
        contextHelpPanelTag("Tile Inspector", "Details for the hovered or selected map tile.", "tile_inspector", contextHelpState);
        if (!viewportPickState.hasHover && !viewportPickState.hasSelection) {
            ImGui::TextDisabled("Hover or click the map viewport to inspect tiles.");
        } else {
            const WorldCoord coord = viewportPickState.hasSelection ? viewportPickState.selectedCoord : viewportPickState.hoveredCoord;
            ImGui::Text("Tile: (%d, %d)", coord.x, coord.y);
            ImGui::Text("In bounds: %s", worldConfig.isWithinWorldBounds(coord) ? "yes" : "no");
            if (worldConfig.isWithinWorldBounds(coord)) {
                const ChunkCoord chunkCoord = worldConfig.worldToChunkCoord(coord);
                const LocalTileCoord localCoord = worldConfig.worldToLocalTileCoord(coord);
                const TerrainId terrainId = chunkStore.getTerrainAt(coord);
                ImGui::Text("Chunk: (%d, %d) index %d", chunkCoord.x, chunkCoord.y, worldConfig.chunkCoordToIndex(chunkCoord));
                ImGui::Text("Local: (%u, %u)", localCoord.x, localCoord.y);
                ImGui::Text("Borough: %s", RegionTable::getRegionName(chunkStore.getRegionAt(coord)).data());
                ImGui::Text("Terrain: %s", getTerrainName(terrainId));
                ImGui::Text("Elevation: %d", chunkStore.getElevationAt(coord));
                ImGui::Text("Chunk active: %s", chunkStore.hasTileAt(coord) ? "yes" : "no");
                ImGui::Separator();
                contextHelpSectionHeader(
                    "Tile Vitality",
                    "Per-tile signals that roll up into borough metrics.",
                    "tile_economic_weight",
                    contextHelpState);
                ImGui::Text("Economic weight: %u", chunkStore.getEconomicWeightAt(coord));
                ImGui::Text("Population: %u", chunkStore.getPopulationAt(coord));
                ImGui::Text("Crime pressure: %u", chunkStore.getCrimePressureAt(coord));
                ImGui::Text("Law pressure: %u", chunkStore.getLawPressureAt(coord));
                ImGui::Text("Business vitality: %u", chunkStore.getBusinessVitalityAt(coord));
                ImGui::Text("Player influence: %u", chunkStore.getPlayerInfluenceAt(coord));
                ImGui::Text("Opposition influence: %u", chunkStore.getOppositionInfluenceAt(coord));
                const RegionId tileRegion = chunkStore.getRegionAt(coord);
                const BoroughVitalitySnapshot* boroughSnapshot = getBoroughSnapshot(boroughVitalityStore, tileRegion);
                if (boroughSnapshot != nullptr && chunkStore.getEconomicWeightAt(coord) > 0) {
                    ImGui::TextDisabled(
                        "Contributes to %s health (%.0f).",
                        RegionTable::getRegionShortName(tileRegion).data(),
                        boroughSnapshot->economicHealth);
                }
            }
        }
    }
    ImGui::End();
}

void updateViewportPickFromWorldCoord(ViewportPickState& viewportPickState, const WorldConfig& worldConfig, const WorldCoord& coord, bool isSelection) {
    if (!worldConfig.isWithinWorldBounds(coord)) {
        viewportPickState.hasHover = false;
        return;
    }
    viewportPickState.hoveredCoord = coord;
    viewportPickState.hasHover = true;
    if (isSelection) {
        viewportPickState.selectedCoord = coord;
        viewportPickState.hasSelection = true;
    }
}

void renderMapViewportPanel(
    const WorldConfig& worldConfig,
    const ChunkStore& chunkStore,
    MapCamera& mapCamera,
    ViewportPickState& viewportPickState,
    bool showCrimeOverlay,
    ContextHelpState& contextHelpState) {
    ImGui::SetNextWindowSizeConstraints(ImVec2(MIN_WINDOW_WIDTH * 0.4f, MIN_WINDOW_HEIGHT * 0.4f), ImVec2(FLT_MAX, FLT_MAX));
    if (ImGui::Begin("Map Viewport")) {
        contextHelpPanelTag("Map Viewport", "Pan, zoom, and pick tiles on the world map.", "map_viewport", contextHelpState);
        const ImVec2 canvasPos = ImGui::GetCursorScreenPos();
        const ImVec2 canvasSize = ImGui::GetContentRegionAvail();
        ImGui::InvisibleButton(
            "map_viewport_canvas",
            canvasSize,
            ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonMiddle | ImGuiButtonFlags_MouseButtonRight);
        const bool isCanvasHovered = ImGui::IsItemHovered();
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        const ImVec2 canvasMax(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y);
        drawList->PushClipRect(canvasPos, canvasMax, true);
        drawList->AddRectFilled(canvasPos, canvasMax, IM_COL32(12, 14, 18, 255));
        if (isCanvasHovered) {
            ImGuiIO& io = ImGui::GetIO();
            if (!contextHelpState.isInspectMode && io.MouseWheel != 0.0f) {
                float focusWorldX = 0.0f;
                float focusWorldY = 0.0f;
                mapCamera.screenToWorld(io.MousePos.x, io.MousePos.y, canvasPos.x, canvasPos.y, canvasSize.x, canvasSize.y, focusWorldX, focusWorldY);
                const float zoomFactor = io.MouseWheel > 0.0f ? ZOOM_WHEEL_FACTOR : (1.0f / ZOOM_WHEEL_FACTOR);
                mapCamera.zoomAt(zoomFactor, focusWorldX, focusWorldY);
            }
            const bool isPanning = !contextHelpState.isInspectMode
                && (ImGui::IsMouseDragging(ImGuiMouseButton_Middle)
                || ImGui::IsMouseDragging(ImGuiMouseButton_Right)
                || ImGui::IsMouseDragging(ImGuiMouseButton_Left));
            if (isPanning) {
                mapCamera.panPixels(io.MouseDelta.x, io.MouseDelta.y);
            } else {
                viewportPickState.hasLandmarkHover = false;
                viewportPickState.hoveredLandmarkIndex = -1;
                const int32_t landmarkIndex = findLandmarkIndexAtScreenPoint(
                    mapCamera,
                    io.MousePos.x,
                    io.MousePos.y,
                    canvasPos,
                    canvasSize,
                    LANDMARK_HIT_RADIUS_PIXELS);
                if (landmarkIndex >= 0) {
                    const LandmarkDefinition* landmark = getLandmarkDefinition(landmarkIndex);
                    if (landmark != nullptr) {
                        viewportPickState.hasLandmarkHover = true;
                        viewportPickState.hoveredLandmarkIndex = landmarkIndex;
                        const WorldCoord landmarkCoord{landmark->tileX, landmark->tileY};
                        updateViewportPickFromWorldCoord(viewportPickState, worldConfig, landmarkCoord, false);
                        ImGui::SetTooltip("%s", getLandmarkTooltipText(landmarkIndex));
                        if (!contextHelpState.isInspectMode && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                            viewportPickState.hasLandmarkSelection = true;
                            viewportPickState.selectedLandmarkIndex = landmarkIndex;
                            updateViewportPickFromWorldCoord(viewportPickState, worldConfig, landmarkCoord, true);
                        }
                    }
                } else {
                    float worldX = 0.0f;
                    float worldY = 0.0f;
                    mapCamera.screenToWorld(io.MousePos.x, io.MousePos.y, canvasPos.x, canvasPos.y, canvasSize.x, canvasSize.y, worldX, worldY);
                    const WorldCoord coord = mapCamera.worldToTile(worldX, worldY);
                    updateViewportPickFromWorldCoord(viewportPickState, worldConfig, coord, false);
                    if (!contextHelpState.isInspectMode && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                        viewportPickState.hasLandmarkSelection = false;
                        viewportPickState.selectedLandmarkIndex = -1;
                        updateViewportPickFromWorldCoord(viewportPickState, worldConfig, coord, true);
                    }
                }
            }
        } else {
            viewportPickState.hasHover = false;
        }
        if (canvasSize.x > 1.0f && canvasSize.y > 1.0f) {
            const WorldCoord* hoveredTileCoord = nullptr;
            if (viewportPickState.hasHover) {
                hoveredTileCoord = &viewportPickState.hoveredCoord;
            }
            renderMapTiles(drawList, mapCamera, worldConfig, chunkStore, canvasPos, canvasSize, hoveredTileCoord, showCrimeOverlay);
            renderLandmarkOverlays(drawList, mapCamera, canvasPos, canvasSize, viewportPickState);
        }
        drawList->PopClipRect();
        if (isCanvasHovered) {
            char overlayBuffer[96];
            std::snprintf(
                overlayBuffer,
                sizeof(overlayBuffer),
                "Scroll: zoom | Drag: pan | Zoom: %.2f px/tile",
                mapCamera.pixelsPerTile);
            const ImVec2 hintSize = ImGui::CalcTextSize(overlayBuffer);
            drawList->AddText(
                ImVec2(canvasPos.x + 8.0f, canvasMax.y - hintSize.y - 8.0f),
                IM_COL32(220, 224, 232, 230),
                overlayBuffer);
        }
    } else {
        viewportPickState.hasHover = false;
    }
    ImGui::End();
}
} // namespace

void setSaveLoadStatusMessage(const char* message) {
    if (message == nullptr) {
        saveLoadStatusMessage[0] = '\0';
        return;
    }
    std::snprintf(saveLoadStatusMessage, sizeof(saveLoadStatusMessage), "%s", message);
}

const char* getSaveLoadStatusMessage() {
    return saveLoadStatusMessage;
}

FrontendUiEvents renderFrontendUi(
    FrontendScreen& frontendScreen,
    CharacterDraft& characterDraft,
    bool hasSaveFile,
    uint64_t worldSeed) {
    FrontendUiEvents frontendUiEvents{};
    switch (frontendScreen) {
    case FrontendScreen::MainMenu:
        renderMainMenuScreen(frontendUiEvents, frontendScreen, hasSaveFile);
        break;
    case FrontendScreen::CharacterCreation:
        renderCharacterCreationScreen(characterDraft, frontendUiEvents, frontendScreen, worldSeed);
        break;
    case FrontendScreen::InGame:
        break;
    default:
        break;
    }
    return frontendUiEvents;
}

void renderGameUi(
    SimClock& simClock,
    const WorldConfig& worldConfig,
    const ChunkStore& chunkStore,
    const BoroughVitalityStore& boroughVitalityStore,
    PlayerWallet& playerWallet,
    CityControlStore& cityControlStore,
    SimEventQueue& simEventQueue,
    bool& mapCrimeOverlayEnabled,
    SystemRegistry& systemRegistry,
    MapCamera& mapCamera,
    ViewportPickState& viewportPickState,
    uint64_t worldSeed,
    const PlayerProfile& playerProfile,
    ContextHelpState& contextHelpState) {
    beginMainDockSpace();
    setupDefaultDockLayoutIfNeeded();
    renderCharacterPanel(playerProfile, playerWallet, contextHelpState);
    renderSimulationPanel(
        simClock,
        worldConfig,
        chunkStore,
        boroughVitalityStore,
        mapCrimeOverlayEnabled,
        systemRegistry,
        worldSeed,
        contextHelpState);
    renderBoroughsPanel(boroughVitalityStore, contextHelpState);
    renderTileInspectorPanel(worldConfig, chunkStore, boroughVitalityStore, viewportPickState, contextHelpState);
    renderCityPanel(
        worldConfig,
        chunkStore,
        boroughVitalityStore,
        cityControlStore,
        playerWallet,
        simEventQueue,
        playerProfile,
        viewportPickState,
        contextHelpState);
    renderMapViewportPanel(worldConfig, chunkStore, mapCamera, viewportPickState, mapCrimeOverlayEnabled, contextHelpState);
}

} // namespace Core
