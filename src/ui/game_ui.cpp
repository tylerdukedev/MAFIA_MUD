#include "ui/map_renderer.h"
#include "ui/game_ui.h"
#include <cstdio>
#include <cfloat>
#include "ui/dock_layout.h"
#include "world/region_table.h"
#include "imgui.h"

namespace Core {

namespace {
constexpr float MIN_WINDOW_WIDTH = 960.0f;
constexpr float MIN_WINDOW_HEIGHT = 540.0f;
constexpr float SPEED_OPTIONS[] = {0.25f, 0.5f, 1.0f, 2.0f, 4.0f};
constexpr int32_t SPEED_OPTION_COUNT = 5;
constexpr float ZOOM_WHEEL_FACTOR = 1.12f;

void renderMainMenu(SimClock& simClock) {
    if (!ImGui::BeginMainMenuBar()) {
        return;
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
}

void renderSimulationPanel(SimClock& simClock, const WorldConfig& worldConfig, const ChunkStore& chunkStore, const SystemRegistry& systemRegistry, uint64_t worldSeed) {
    ImGui::SetNextWindowSizeConstraints(ImVec2(240.0f, 200.0f), ImVec2(FLT_MAX, FLT_MAX));
    if (ImGui::Begin("Simulation")) {
        ImGui::Text("Phase 4 — Map Renderer");
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
        ImGui::Text("Regions: %d", RegionTable::getPlayableRegionCount());
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

void renderRegionsPanel() {
    ImGui::SetNextWindowSizeConstraints(ImVec2(220.0f, 160.0f), ImVec2(FLT_MAX, FLT_MAX));
    if (ImGui::Begin("Regions")) {
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
                ImGui::Text("Chunk: (%d, %d) index %d", chunkCoord.x, chunkCoord.y, worldConfig.chunkCoordToIndex(chunkCoord));
                ImGui::Text("Local: (%u, %u)", localCoord.x, localCoord.y);
                ImGui::Text("Region: %s", RegionTable::getRegionName(chunkStore.getRegionAt(coord)).data());
                ImGui::Text("Terrain: %u", static_cast<unsigned>(chunkStore.getTerrainAt(coord)));
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
        bool showTilePreview = false;
        WorldCoord previewCoord{};
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
                if (worldConfig.isWithinWorldBounds(coord)) {
                    showTilePreview = true;
                    previewCoord = coord;
                }
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    updateViewportPickFromWorldCoord(viewportPickState, worldConfig, coord, true);
                }
            }
        } else {
            viewportPickState.hasHover = false;
        }
        if (canvasSize.x > 1.0f && canvasSize.y > 1.0f) {
            renderMapTiles(drawList, mapCamera, worldConfig, chunkStore, canvasPos, canvasSize);
            if (showTilePreview) {
                renderHoveredTilePreview(drawList, worldConfig, chunkStore, previewCoord, canvasPos, canvasSize);
            }
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

void renderGameUi(
    SimClock& simClock,
    const WorldConfig& worldConfig,
    const ChunkStore& chunkStore,
    SystemRegistry& systemRegistry,
    MapCamera& mapCamera,
    ViewportPickState& viewportPickState,
    uint64_t worldSeed) {
    renderMainMenu(simClock);
    beginMainDockSpace();
    renderSimulationPanel(simClock, worldConfig, chunkStore, systemRegistry, worldSeed);
    renderRegionsPanel();
    renderTileInspectorPanel(worldConfig, chunkStore, viewportPickState);
    renderMapViewportPanel(worldConfig, chunkStore, mapCamera, viewportPickState);
}

} // namespace Core
