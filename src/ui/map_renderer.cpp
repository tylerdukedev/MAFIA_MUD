#include "ui/map_renderer.h"
#include "imgui.h"
#include <algorithm>
#include <cmath>

namespace Core {

namespace {
ImU32 scaleColor(ImU32 color, float factor) {
    const float red = static_cast<float>((color >> IM_COL32_R_SHIFT) & 0xFF) * factor;
    const float green = static_cast<float>((color >> IM_COL32_G_SHIFT) & 0xFF) * factor;
    const float blue = static_cast<float>((color >> IM_COL32_B_SHIFT) & 0xFF) * factor;
    const int clampedRed = static_cast<int>(std::min(255.0f, std::max(0.0f, red)));
    const int clampedGreen = static_cast<int>(std::min(255.0f, std::max(0.0f, green)));
    const int clampedBlue = static_cast<int>(std::min(255.0f, std::max(0.0f, blue)));
    return IM_COL32(clampedRed, clampedGreen, clampedBlue, 255);
}

float alignLineCoord(float value) {
    return std::floor(value) + 0.5f;
}

ImU32 getBoroughColor(RegionId regionId) {
    switch (regionId) {
    case RegionId::Manhattan: return IM_COL32(86, 180, 80, 255);
    case RegionId::Brooklyn: return IM_COL32(240, 220, 70, 255);
    case RegionId::Queens: return IM_COL32(242, 140, 46, 255);
    case RegionId::Bronx: return IM_COL32(224, 66, 60, 255);
    case RegionId::StatenIsland: return IM_COL32(158, 98, 184, 255);
    case RegionId::NewJersey:
    case RegionId::LongIsland: return IM_COL32(225, 200, 158, 255);
    default: return IM_COL32(120, 124, 132, 255);
    }
}
} // namespace

ImU32 getTileColor(RegionId regionId, TerrainId terrainId, int16_t elevation) {
    if (terrainId == TerrainId::Water) {
        return IM_COL32(148, 204, 232, 255);
    }
    if (terrainId == TerrainId::Park) {
        return IM_COL32(84, 160, 76, 255);
    }
    if (terrainId == TerrainId::Plaza) {
        return IM_COL32(204, 200, 190, 255);
    }
    if (terrainId == TerrainId::OpenLand) {
        return getBoroughColor(regionId);
    }
    const ImU32 boroughColor = getBoroughColor(regionId);
    if (terrainId == TerrainId::Road) {
        return scaleColor(boroughColor, 0.60f);
    }
    const float tint = 0.88f + std::min(static_cast<float>(elevation) / 255.0f, 1.0f) * 0.22f;
    return scaleColor(boroughColor, tint);
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
            const int16_t elevation = chunkStore.getElevationAt(coord);
            const ImU32 tileColor = getTileColor(regionId, terrainId, elevation);
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
            if (tileSizePixels >= 4.0f && terrainId != TerrainId::Water) {
                const float inset = std::min(1.0f, tileSizePixels * 0.14f);
                const ImU32 highlightColor = scaleColor(tileColor, 1.10f);
                drawList->AddRectFilled(
                    ImVec2(screenMinX + inset, screenMinY + inset),
                    ImVec2(screenMaxX - inset, screenMaxY - inset),
                    highlightColor);
            }
            if (tileSizePixels >= 3.5f && terrainId != TerrainId::Water) {
                const ImU32 gridColor = IM_COL32(255, 255, 255, 24);
                const float topY = alignLineCoord(screenMinY);
                const float leftX = alignLineCoord(screenMinX);
                const float rightX = alignLineCoord(screenMaxX);
                const float bottomY = alignLineCoord(screenMaxY);
                drawList->AddLine(ImVec2(leftX, topY), ImVec2(rightX, topY), gridColor, 1.0f);
                drawList->AddLine(ImVec2(leftX, topY), ImVec2(leftX, bottomY), gridColor, 1.0f);
            }
        }
    }
}

} // namespace Core
