#include "ui/map_renderer.h"
#include "imgui.h"
#include <algorithm>
#include <cmath>

namespace Core {

ImU32 getRegionColor(RegionId regionId, TerrainId terrainId) {
    if (terrainId == TerrainId::Water) {
        return IM_COL32(28, 58, 92, 255);
    }
    switch (regionId) {
    case RegionId::Manhattan: return IM_COL32(196, 196, 204, 255);
    case RegionId::Brooklyn: return IM_COL32(168, 118, 78, 255);
    case RegionId::Queens: return IM_COL32(96, 148, 88, 255);
    case RegionId::Bronx: return IM_COL32(176, 150, 108, 255);
    case RegionId::StatenIsland: return IM_COL32(140, 112, 168, 255);
    case RegionId::NewJersey: return IM_COL32(108, 128, 156, 255);
    default: return IM_COL32(64, 68, 76, 255);
    }
}

void renderMapTiles(
    ImDrawList* drawList,
    const MapCamera& camera,
    const WorldConfig& worldConfig,
    const ChunkStore& chunkStore,
    const ImVec2& canvasOrigin,
    const ImVec2& canvasSize) {
    const float canvasWidthPixels = canvasSize.x;
    const float canvasHeightPixels = canvasSize.y;
    if (canvasWidthPixels <= 1.0f || canvasHeightPixels <= 1.0f) {
        return;
    }
    const float halfWorldWidth = (canvasWidthPixels * 0.5f / camera.pixelsPerTile) + 2.0f;
    const float halfWorldHeight = (canvasHeightPixels * 0.5f / camera.pixelsPerTile) + 2.0f;
    const int32_t startX = static_cast<int32_t>(std::floor(camera.centerWorldX - halfWorldWidth));
    const int32_t endX = static_cast<int32_t>(std::ceil(camera.centerWorldX + halfWorldWidth));
    const int32_t startY = static_cast<int32_t>(std::floor(camera.centerWorldY - halfWorldHeight));
    const int32_t endY = static_cast<int32_t>(std::ceil(camera.centerWorldY + halfWorldHeight));
    const int32_t clampedStartX = std::max(0, startX);
    const int32_t clampedEndX = std::min(worldConfig.WORLD_WIDTH_TILES - 1, endX);
    const int32_t clampedStartY = std::max(0, startY);
    const int32_t clampedEndY = std::min(worldConfig.WORLD_HEIGHT_TILES - 1, endY);
    const float tileSizePixels = std::max(camera.pixelsPerTile, 1.0f);
    for (int32_t tileY = clampedStartY; tileY <= clampedEndY; ++tileY) {
        for (int32_t tileX = clampedStartX; tileX <= clampedEndX; ++tileX) {
            WorldCoord coord{tileX, tileY};
            const TerrainId terrainId = chunkStore.getTerrainAt(coord);
            const RegionId regionId = chunkStore.getRegionAt(coord);
            const ImU32 tileColor = getRegionColor(regionId, terrainId);
            float screenMinX = 0.0f;
            float screenMinY = 0.0f;
            float screenMaxX = 0.0f;
            float screenMaxY = 0.0f;
            camera.tileToScreen(static_cast<float>(tileX), static_cast<float>(tileY), canvasOrigin.x, canvasOrigin.y, canvasWidthPixels, canvasHeightPixels, screenMinX, screenMinY);
            camera.tileToScreen(static_cast<float>(tileX + 1), static_cast<float>(tileY + 1), canvasOrigin.x, canvasOrigin.y, canvasWidthPixels, canvasHeightPixels, screenMaxX, screenMaxY);
            if (screenMaxX < canvasOrigin.x || screenMinX > canvasOrigin.x + canvasWidthPixels) {
                continue;
            }
            if (screenMaxY < canvasOrigin.y || screenMinY > canvasOrigin.y + canvasHeightPixels) {
                continue;
            }
            drawList->AddRectFilled(ImVec2(screenMinX, screenMinY), ImVec2(screenMaxX, screenMaxY), tileColor);
            if (tileSizePixels >= 6.0f) {
                drawList->AddRect(ImVec2(screenMinX, screenMinY), ImVec2(screenMaxX, screenMaxY), IM_COL32(20, 22, 26, 80));
            }
        }
    }
}

} // namespace Core
