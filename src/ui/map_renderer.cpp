#include "ui/map_renderer.h"
#include "imgui.h"
#include <algorithm>
#include <cmath>

namespace Core {

namespace {
constexpr int32_t PREVIEW_TILE_RADIUS = 4;
constexpr float PREVIEW_TILE_PIXELS = 30.0f;
constexpr float PREVIEW_PERSPECTIVE_SKEW = 0.48f;

ImU32 scaleColor(ImU32 color, float factor) {
    const float red = static_cast<float>((color >> IM_COL32_R_SHIFT) & 0xFF) * factor;
    const float green = static_cast<float>((color >> IM_COL32_G_SHIFT) & 0xFF) * factor;
    const float blue = static_cast<float>((color >> IM_COL32_B_SHIFT) & 0xFF) * factor;
    const int clampedRed = static_cast<int>(std::min(255.0f, std::max(0.0f, red)));
    const int clampedGreen = static_cast<int>(std::min(255.0f, std::max(0.0f, green)));
    const int clampedBlue = static_cast<int>(std::min(255.0f, std::max(0.0f, blue)));
    return IM_COL32(clampedRed, clampedGreen, clampedBlue, 255);
}

void drawPreviewTile(
    ImDrawList* drawList,
    const ImVec2& topLeft,
    const ImVec2& topRight,
    const ImVec2& bottomRight,
    const ImVec2& bottomLeft,
    ImU32 fillColor,
    ImU32 borderColor) {
    drawList->AddQuadFilled(topLeft, topRight, bottomRight, bottomLeft, fillColor);
    drawList->AddQuad(topLeft, topRight, bottomRight, bottomLeft, borderColor, 1.5f);
}
} // namespace

ImU32 getTileColor(RegionId regionId, TerrainId terrainId, int16_t elevation) {
    (void)regionId;
    const float shade = 0.84f + std::min(static_cast<float>(elevation) / 255.0f, 1.0f) * 0.32f;
    switch (terrainId) {
    case TerrainId::DeepWater: return IM_COL32(28, 56, 110, 255);
    case TerrainId::ShallowWater: return IM_COL32(54, 104, 168, 255);
    case TerrainId::River: return IM_COL32(64, 126, 196, 255);
    case TerrainId::Beach: return IM_COL32(214, 200, 150, 255);
    case TerrainId::Grassland: return scaleColor(IM_COL32(104, 156, 78, 255), shade);
    case TerrainId::Forest: return scaleColor(IM_COL32(46, 98, 54, 255), shade);
    case TerrainId::Hills: return scaleColor(IM_COL32(132, 138, 86, 255), shade);
    case TerrainId::Mountain: return scaleColor(IM_COL32(126, 118, 108, 255), shade);
    case TerrainId::Peak: return IM_COL32(232, 234, 240, 255);
    case TerrainId::City: return IM_COL32(198, 178, 150, 255);
    case TerrainId::Road: return IM_COL32(122, 96, 66, 255);
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
            if (tileSizePixels >= 5.0f && terrainId == TerrainId::City) {
                const float inset = std::min(tileSizePixels * 0.18f, 2.0f);
                drawList->AddRect(
                    ImVec2(screenMinX + inset, screenMinY + inset),
                    ImVec2(screenMaxX - inset, screenMaxY - inset),
                    scaleColor(tileColor, 0.7f),
                    0.0f,
                    0,
                    1.0f);
            }
        }
    }
}

void renderHoveredTilePreview(
    ImDrawList* drawList,
    const WorldConfig& worldConfig,
    const ChunkStore& chunkStore,
    const WorldCoord& centerCoord,
    const ImVec2& canvasOrigin,
    const ImVec2& canvasSize) {
    if (!worldConfig.isWithinWorldBounds(centerCoord)) {
        return;
    }
    const float previewWidth = std::min(canvasSize.x * 0.48f, 340.0f);
    const float previewHeight = std::min(canvasSize.y * 0.52f, 360.0f);
    const ImVec2 previewOrigin(canvasOrigin.x + 12.0f, canvasOrigin.y + 12.0f);
    const ImVec2 previewMax(previewOrigin.x + previewWidth, previewOrigin.y + previewHeight);
    drawList->AddRectFilled(previewOrigin, previewMax, IM_COL32(16, 18, 24, 230));
    drawList->AddRect(previewOrigin, previewMax, IM_COL32(220, 224, 232, 200), 0.0f, 0, 2.0f);
    const float gridWidth = static_cast<float>(PREVIEW_TILE_RADIUS * 2 + 1) * PREVIEW_TILE_PIXELS;
    const float gridHeight = gridWidth * (1.0f - PREVIEW_PERSPECTIVE_SKEW * 0.25f);
    const ImVec2 gridOrigin(
        previewOrigin.x + (previewWidth - gridWidth) * 0.5f,
        previewOrigin.y + (previewHeight - gridHeight) * 0.52f);
    for (int32_t offsetY = -PREVIEW_TILE_RADIUS; offsetY <= PREVIEW_TILE_RADIUS; ++offsetY) {
        for (int32_t offsetX = -PREVIEW_TILE_RADIUS; offsetX <= PREVIEW_TILE_RADIUS; ++offsetX) {
            WorldCoord coord{centerCoord.x + offsetX, centerCoord.y + offsetY};
            if (!worldConfig.isWithinWorldBounds(coord)) {
                continue;
            }
            const TerrainId terrainId = chunkStore.getTerrainAt(coord);
            const RegionId regionId = chunkStore.getRegionAt(coord);
            const int16_t elevation = chunkStore.getElevationAt(coord);
            const ImU32 fillColor = getTileColor(regionId, terrainId, elevation);
            const ImU32 borderColor = scaleColor(fillColor, 0.55f);
            const float depth = static_cast<float>(offsetY + PREVIEW_TILE_RADIUS);
            const float scale = 1.0f + (static_cast<float>(PREVIEW_TILE_RADIUS) - std::abs(static_cast<float>(offsetX))) * 0.04f
                + (static_cast<float>(PREVIEW_TILE_RADIUS) - std::abs(static_cast<float>(offsetY))) * 0.06f;
            const float tilePixels = PREVIEW_TILE_PIXELS * scale;
            const float skewShift = depth * PREVIEW_TILE_PIXELS * PREVIEW_PERSPECTIVE_SKEW;
            const float baseX = gridOrigin.x + static_cast<float>(offsetX + PREVIEW_TILE_RADIUS) * PREVIEW_TILE_PIXELS + skewShift;
            const float baseY = gridOrigin.y + depth * PREVIEW_TILE_PIXELS * (1.0f - PREVIEW_PERSPECTIVE_SKEW * 0.35f);
            const ImVec2 topLeft(baseX, baseY);
            const ImVec2 topRight(baseX + tilePixels, baseY - tilePixels * PREVIEW_PERSPECTIVE_SKEW * 0.15f);
            const ImVec2 bottomLeft(baseX + tilePixels * PREVIEW_PERSPECTIVE_SKEW * 0.12f, baseY + tilePixels);
            const ImVec2 bottomRight(baseX + tilePixels, baseY + tilePixels * (1.0f - PREVIEW_PERSPECTIVE_SKEW * 0.12f));
            drawPreviewTile(drawList, topLeft, topRight, bottomRight, bottomLeft, fillColor, borderColor);
            if (offsetX == 0 && offsetY == 0) {
                drawList->AddQuad(
                    topLeft,
                    topRight,
                    bottomRight,
                    bottomLeft,
                    IM_COL32(255, 255, 255, 240),
                    2.5f);
            }
        }
    }
}

} // namespace Core
