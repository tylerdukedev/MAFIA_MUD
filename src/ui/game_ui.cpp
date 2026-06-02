#include "ui/game_ui.h"
#include "world/region_table.h"
#include "imgui.h"

namespace Core {

void renderGameUi(SimClock& simClock, const WorldConfig& worldConfig, const ChunkStore& chunkStore) {
    if (ImGui::BeginMainMenuBar()) {
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
            ImGui::MenuItem("Debug Panel", nullptr, nullptr);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
    ImGui::SetNextWindowSize(ImVec2(360.0f, 280.0f), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("NYC Systems Sim")) {
        ImGui::Text("Phase 0-1 — Bootstrap + World Skeleton");
        ImGui::Separator();
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        ImGui::Text("Sim: %s", simClock.isPaused() ? "PAUSED" : "RUNNING");
        ImGui::Text("Tick rate: %.0f Hz", simClock.getTickRateHz());
        ImGui::Text("Tick count: %llu", static_cast<unsigned long long>(simClock.getTickCount()));
        ImGui::Text("Ticks this frame: %d", simClock.getTicksThisFrame());
        ImGui::Separator();
        ImGui::Text("World: %d x %d tiles", worldConfig.WORLD_WIDTH_TILES, worldConfig.WORLD_HEIGHT_TILES);
        ImGui::Text("Chunk size: %d x %d", worldConfig.CHUNK_SIZE, worldConfig.CHUNK_SIZE);
        ImGui::Text("Chunks: %d x %d (%d total)", worldConfig.CHUNK_COUNT_X, worldConfig.CHUNK_COUNT_Y, chunkStore.getTotalChunkCount());
        ImGui::Text("Active chunks: %d", chunkStore.getActiveChunkCount());
        ImGui::Text("Regions: %d", RegionTable::getPlayableRegionCount());
        ImGui::Separator();
        ImGui::TextDisabled("Space = pause/resume | S = step tick");
    }
    ImGui::End();
    ImGui::SetNextWindowSize(ImVec2(320.0f, 200.0f), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Regions")) {
        for (int32_t regionIndex = 1; regionIndex < static_cast<int32_t>(RegionId::COUNT); ++regionIndex) {
            const auto regionId = static_cast<RegionId>(regionIndex);
            ImGui::BulletText("%s (%s)", RegionTable::getRegionName(regionId).data(), RegionTable::getRegionShortName(regionId).data());
        }
    }
    ImGui::End();
}

} // namespace Core
