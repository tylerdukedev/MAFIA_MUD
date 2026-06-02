#include "ui/map_renderer.h"
#include "ui/game_ui.h"
#include "character/character_tables.h"
#include "character/profile_builder.h"
#include <cstdio>
#include <cfloat>
#include <cstring>
#include "ui/dock_layout.h"
#include "world/region_table.h"
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
        "Starting Borough Preference",
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

void renderCharacterCreationScreen(CharacterDraft& characterDraft, FrontendUiEvents& frontendUiEvents, FrontendScreen& frontendScreen) {
    initializeCharacterDraftDefaults(characterDraft);
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

GameUiEvents renderInGameMenuBar(SimClock& simClock, bool hasSaveFile) {
    GameUiEvents gameUiEvents{};
    if (!ImGui::BeginMainMenuBar()) {
        return gameUiEvents;
    }
    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("Save Game", "Ctrl+S")) {
            gameUiEvents.requestedSaveGame = true;
        }
        if (!hasSaveFile) {
            ImGui::BeginDisabled();
        }
        if (ImGui::MenuItem("Load Game")) {
            gameUiEvents.requestedLoadGame = true;
        }
        if (!hasSaveFile) {
            ImGui::EndDisabled();
        }
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Simulation")) {
        if (ImGui::MenuItem(simClock.isPaused() ? "Resume" : "Pause", "Space")) {
            simClock.togglePaused();
        }
        if (ImGui::MenuItem("Step One Tick", "S")) {
            simClock.stepOneTick();
        }
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("View")) {
        if (ImGui::MenuItem("Reset Panel Layout")) {
            resetDockLayout();
        }
        ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
    return gameUiEvents;
}

void renderSimulationPanel(SimClock& simClock, const WorldConfig& worldConfig, const ChunkStore& chunkStore, const SystemRegistry& systemRegistry, uint64_t worldSeed) {
    ImGui::SetNextWindowSizeConstraints(ImVec2(240.0f, 200.0f), ImVec2(FLT_MAX, FLT_MAX));
    if (ImGui::Begin("Simulation")) {
        if (saveLoadStatusMessage[0] != '\0') {
            ImGui::TextWrapped("%s", saveLoadStatusMessage);
            ImGui::Separator();
        }
        ImGui::Text("Phase 5 — Five Boroughs");
        ImGui::Text("World seed: %llu", static_cast<unsigned long long>(worldSeed));
        ImGui::Separator();
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        ImGui::Text("Window: %.0f x %.0f", ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y);
        ImGui::Text("Sim: %s", simClock.isPaused() ? "PAUSED" : "RUNNING");
        ImGui::Text("Tick rate: %.0f Hz", simClock.getTickRateHz());
        ImGui::Text("Speed: %.2fx", simClock.getSpeedMultiplier());
        ImGui::Text("Tick count: %llu", static_cast<unsigned long long>(simClock.getTickCount()));
        ImGui::Text("Ticks this frame: %d", simClock.getTicksThisFrame());
        ImGui::Separator();
        ImGui::Text("Speed multiplier");
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
        ImGui::Separator();
        ImGui::Text("World: %d x %d tiles", worldConfig.WORLD_WIDTH_TILES, worldConfig.WORLD_HEIGHT_TILES);
        ImGui::Text("Chunk size: %d x %d", worldConfig.CHUNK_SIZE, worldConfig.CHUNK_SIZE);
        ImGui::Text("Chunks: %d x %d (%d total)", worldConfig.CHUNK_COUNT_X, worldConfig.CHUNK_COUNT_Y, chunkStore.getTotalChunkCount());
        ImGui::Text("Active chunks: %d", chunkStore.getActiveChunkCount());
        ImGui::Text("Boroughs: %d", RegionTable::getPlayableRegionCount());
        ImGui::Separator();
        ImGui::Text("Systems: %d", systemRegistry.getSystemCount());
        for (int32_t index = 0; index < systemRegistry.getSystemCount(); ++index) {
            const ISimSystem* system = systemRegistry.getSystem(index);
            if (system != nullptr) {
                ImGui::BulletText("%s (tick %llu)", system->getName(), static_cast<unsigned long long>(system->getLastTickCount()));
            }
        }
        ImGui::Separator();
        ImGui::TextDisabled("Space = pause/resume | S = step tick");
    }
    ImGui::End();
}

void renderBoroughsPanel() {
    ImGui::SetNextWindowSizeConstraints(ImVec2(220.0f, 160.0f), ImVec2(FLT_MAX, FLT_MAX));
    if (ImGui::Begin("Boroughs")) {
        for (int32_t regionIndex = 1; regionIndex < static_cast<int32_t>(RegionId::COUNT); ++regionIndex) {
            const auto regionId = static_cast<RegionId>(regionIndex);
            ImGui::BulletText("%s (%s)", RegionTable::getRegionName(regionId).data(), RegionTable::getRegionShortName(regionId).data());
        }
    }
    ImGui::End();
}

void renderTileInspectorPanel(const WorldConfig& worldConfig, const ChunkStore& chunkStore, const ViewportPickState& viewportPickState) {
    ImGui::SetNextWindowSizeConstraints(ImVec2(320.0f, 140.0f), ImVec2(FLT_MAX, FLT_MAX));
    if (ImGui::Begin("Tile Inspector")) {
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
    ViewportPickState& viewportPickState) {
    ImGui::SetNextWindowSizeConstraints(ImVec2(MIN_WINDOW_WIDTH * 0.4f, MIN_WINDOW_HEIGHT * 0.4f), ImVec2(FLT_MAX, FLT_MAX));
    if (ImGui::Begin("Map Viewport")) {
        static bool isCameraInitialized = false;
        const ImVec2 canvasPos = ImGui::GetCursorScreenPos();
        const ImVec2 canvasSize = ImGui::GetContentRegionAvail();
        if (!isCameraInitialized && canvasSize.x > 1.0f && canvasSize.y > 1.0f) {
            mapCamera.fitToWorld(worldConfig.WORLD_WIDTH_TILES, worldConfig.WORLD_HEIGHT_TILES, canvasSize.x, canvasSize.y);
            isCameraInitialized = true;
        }
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
            if (io.MouseWheel != 0.0f) {
                float focusWorldX = 0.0f;
                float focusWorldY = 0.0f;
                mapCamera.screenToWorld(io.MousePos.x, io.MousePos.y, canvasPos.x, canvasPos.y, canvasSize.x, canvasSize.y, focusWorldX, focusWorldY);
                const float zoomFactor = io.MouseWheel > 0.0f ? ZOOM_WHEEL_FACTOR : (1.0f / ZOOM_WHEEL_FACTOR);
                mapCamera.zoomAt(zoomFactor, focusWorldX, focusWorldY);
            }
            const bool isPanning = ImGui::IsMouseDragging(ImGuiMouseButton_Middle)
                || ImGui::IsMouseDragging(ImGuiMouseButton_Right)
                || ImGui::IsMouseDragging(ImGuiMouseButton_Left);
            if (isPanning) {
                mapCamera.panPixels(io.MouseDelta.x, io.MouseDelta.y);
            } else {
                float worldX = 0.0f;
                float worldY = 0.0f;
                mapCamera.screenToWorld(io.MousePos.x, io.MousePos.y, canvasPos.x, canvasPos.y, canvasSize.x, canvasSize.y, worldX, worldY);
                const WorldCoord coord = mapCamera.worldToTile(worldX, worldY);
                updateViewportPickFromWorldCoord(viewportPickState, worldConfig, coord, false);
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    updateViewportPickFromWorldCoord(viewportPickState, worldConfig, coord, true);
                }
            }
        } else {
            viewportPickState.hasHover = false;
        }
        if (canvasSize.x > 1.0f && canvasSize.y > 1.0f) {
            renderMapTiles(drawList, mapCamera, worldConfig, chunkStore, canvasPos, canvasSize);
        }
        if (viewportPickState.hasHover && mapCamera.pixelsPerTile >= 2.0f) {
            const WorldCoord hovered = viewportPickState.hoveredCoord;
            float hoverMinX = 0.0f;
            float hoverMinY = 0.0f;
            float hoverMaxX = 0.0f;
            float hoverMaxY = 0.0f;
            mapCamera.tileToScreen(static_cast<float>(hovered.x), static_cast<float>(hovered.y), canvasPos.x, canvasPos.y, canvasSize.x, canvasSize.y, hoverMinX, hoverMinY);
            mapCamera.tileToScreen(static_cast<float>(hovered.x + 1), static_cast<float>(hovered.y + 1), canvasPos.x, canvasPos.y, canvasSize.x, canvasSize.y, hoverMaxX, hoverMaxY);
            drawList->AddRect(ImVec2(hoverMinX, hoverMinY), ImVec2(hoverMaxX, hoverMaxY), IM_COL32(255, 255, 255, 235), 0.0f, 0, 2.0f);
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

FrontendUiEvents renderFrontendUi(FrontendScreen& frontendScreen, CharacterDraft& characterDraft, bool hasSaveFile) {
    FrontendUiEvents frontendUiEvents{};
    switch (frontendScreen) {
    case FrontendScreen::MainMenu:
        renderMainMenuScreen(frontendUiEvents, frontendScreen, hasSaveFile);
        break;
    case FrontendScreen::CharacterCreation:
        renderCharacterCreationScreen(characterDraft, frontendUiEvents, frontendScreen);
        break;
    case FrontendScreen::InGame:
        break;
    default:
        break;
    }
    return frontendUiEvents;
}

GameUiEvents renderGameUi(
    SimClock& simClock,
    const WorldConfig& worldConfig,
    const ChunkStore& chunkStore,
    SystemRegistry& systemRegistry,
    MapCamera& mapCamera,
    ViewportPickState& viewportPickState,
    uint64_t worldSeed,
    bool hasSaveFile) {
    GameUiEvents gameUiEvents = renderInGameMenuBar(simClock, hasSaveFile);
    beginMainDockSpace();
    renderSimulationPanel(simClock, worldConfig, chunkStore, systemRegistry, worldSeed);
    renderBoroughsPanel();
    renderTileInspectorPanel(worldConfig, chunkStore, viewportPickState);
    renderMapViewportPanel(worldConfig, chunkStore, mapCamera, viewportPickState);
    return gameUiEvents;
}

} // namespace Core
