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

void renderSimulationPanel(SimClock& simClock, const WorldConfig& worldConfig, const ChunkStore& chunkStore, const SystemRegistry& systemRegistry) {
    ImGui::SetNextWindowSizeConstraints(ImVec2(240.0f, 200.0f), ImVec2(FLT_MAX, FLT_MAX));
    if (ImGui::Begin("Simulation")) {
        ImGui::Text("Phase 2 — Sim Loop + Debug UI");
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
                ImGui::Text("Chunk active: %s", chunkStore.hasTileAt(coord) ? "yes" : "no");
            }
        }
    }
    ImGui::End();
}

void renderMapViewportPanel(const WorldConfig& worldConfig, ViewportPickState& viewportPickState) {
    ImGui::SetNextWindowSizeConstraints(ImVec2(MIN_WINDOW_WIDTH * 0.4f, MIN_WINDOW_HEIGHT * 0.4f), ImVec2(FLT_MAX, FLT_MAX));
    if (ImGui::Begin("Map Viewport")) {
        ImGui::TextDisabled("Map renderer arrives in Phase 4. Drag panels to dock edges to rearrange.");
        const ImVec2 canvasPos = ImGui::GetCursorScreenPos();
        const ImVec2 canvasSize = ImGui::GetContentRegionAvail();
        if (canvasSize.x > 1.0f && canvasSize.y > 1.0f) {
            ImGui::InvisibleButton("map_viewport_canvas", canvasSize, ImGuiButtonFlags_MouseButtonLeft);
            const bool isCanvasHovered = ImGui::IsItemHovered();
            const ImU32 backgroundColor = ImGui::GetColorU32(ImVec4(0.06f, 0.07f, 0.09f, 1.0f));
            const ImU32 borderColor = ImGui::GetColorU32(ImVec4(0.22f, 0.24f, 0.28f, 1.0f));
            ImGui::GetWindowDrawList()->AddRectFilled(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), backgroundColor);
            ImGui::GetWindowDrawList()->AddRect(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), borderColor);
            viewportPickState.hasHover = false;
            if (isCanvasHovered) {
                const ImVec2 mousePos = ImGui::GetIO().MousePos;
                const float normalizedX = (mousePos.x - canvasPos.x) / canvasSize.x;
                const float normalizedY = (mousePos.y - canvasPos.y) / canvasSize.y;
                if (normalizedX >= 0.0f && normalizedX <= 1.0f && normalizedY >= 0.0f && normalizedY <= 1.0f) {
                    WorldCoord coord;
                    coord.x = static_cast<int32_t>(normalizedX * static_cast<float>(worldConfig.WORLD_WIDTH_TILES - 1));
                    coord.y = static_cast<int32_t>(normalizedY * static_cast<float>(worldConfig.WORLD_HEIGHT_TILES - 1));
                    viewportPickState.hoveredCoord = coord;
                    viewportPickState.hasHover = true;
                    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                        viewportPickState.selectedCoord = coord;
                        viewportPickState.hasSelection = true;
                    }
                    char overlayBuffer[64];
                    std::snprintf(overlayBuffer, sizeof(overlayBuffer), "(%d, %d)", coord.x, coord.y);
                    ImGui::GetWindowDrawList()->AddText(ImVec2(canvasPos.x + 8.0f, canvasPos.y + 8.0f), ImGui::GetColorU32(ImVec4(0.85f, 0.88f, 0.92f, 1.0f)), "Hover tile");
                    ImGui::GetWindowDrawList()->AddText(ImVec2(canvasPos.x + 8.0f, canvasPos.y + 24.0f), ImGui::GetColorU32(ImVec4(0.65f, 0.70f, 0.78f, 1.0f)), overlayBuffer);
                }
            }
        } else {
            viewportPickState.hasHover = false;
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
    ChunkStore& chunkStore,
    SystemRegistry& systemRegistry,
    ViewportPickState& viewportPickState) {
    renderMainMenu(simClock);
    beginMainDockSpace();
    renderSimulationPanel(simClock, worldConfig, chunkStore, systemRegistry);
    renderRegionsPanel();
    renderTileInspectorPanel(worldConfig, chunkStore, viewportPickState);
    renderMapViewportPanel(worldConfig, viewportPickState);
}

} // namespace Core
