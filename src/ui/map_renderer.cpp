#include "ui/map_renderer.h"
#include "imgui.h"
#include <algorithm>
#include <cmath>

namespace Core {

namespace {
constexpr int32_t PREVIEW_TILE_RADIUS = 4;
constexpr float PREVIEW_TILE_PIXELS = 30.0f;
constexpr float PREVIEW_PERSPECTIVE_SKEW = 0.48f;

ImU32 darkenColor(ImU32 color, float factor) {
    const int red = static_cast<int>(static_cast<float>((color >> IM_COL32_R_SHIFT) & 0xFF) * factor);
    const int green = static_cast<int>(static_cast<float>((color >> IM_COL32_G_SHIFT) & 0xFF) * factor);
    const int blue = static_cast<int>(static_cast<float>((color >> IM_COL32_B_SHIFT) & 0xFF) * factor);
    return IM_COL32(red, green, blue, 255);
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

ImU32 getRegionColor(RegionId regionId, TerrainId terrainId) {
    if (terrainId == TerrainId::Water) {
        return IM_COL32(148, 204, 232, 255);
    }
    switch (regionId) {
    case RegionId::Manhattan: return IM_COL32(96, 188, 88, 255);
    case RegionId::Brooklyn: return IM_COL32(238, 216, 62, 255);
    case RegionId::Queens: return IM_COL32(238, 128, 42, 255);
    case RegionId::Bronx: return IM_COL32(218, 62, 58, 255);
    case RegionId::StatenIsland: return IM_COL32(152, 102, 182, 255);
    case RegionId::NewJersey: return IM_COL32(208, 188, 148, 255);
    default: return IM_COL32(120, 124, 132, 255);
    }
}

ImU32 getRegionBorderColor(RegionId regionId, TerrainId terrainId) {
    return darkenColor(getRegionColor(regionId, terrainId), 0.55f);
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
            if (tileSizePixels >= 4.0f) {
                const RegionId eastRegion = tileX + 1 < worldConfig.WORLD_WIDTH_TILES
                    ? chunkStore.getRegionAt(WorldCoord{tileX + 1, tileY}) : regionId;
                const RegionId southRegion = tileY + 1 < worldConfig.WORLD_HEIGHT_TILES
                    ? chunkStore.getRegionAt(WorldCoord{tileX, tileY + 1}) : regionId;
                if (eastRegion != regionId || terrainId != chunkStore.getTerrainAt(WorldCoord{tileX + 1, tileY})) {
                    drawList->AddLine(ImVec2(screenMaxX, screenMinY), ImVec2(screenMaxX, screenMaxY), IM_COL32(30, 34, 40, 180), 1.0f);
                }
                if (southRegion != regionId || terrainId != chunkStore.getTerrainAt(WorldCoord{tileX, tileY + 1})) {
                    drawList->AddLine(ImVec2(screenMinX, screenMaxY), ImVec2(screenMaxX, screenMaxY), IM_COL32(30, 34, 40, 180), 1.0f);
                }
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
            const ImU32 fillColor = getRegionColor(regionId, terrainId);
            const ImU32 borderColor = getRegionBorderColor(regionId, terrainId);
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
