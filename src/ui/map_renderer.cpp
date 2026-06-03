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
constexpr float MAP_ROAD_COLOR_SCALE = 0.94f;
constexpr float MAP_GRID_EDGE_SCALE = 0.90f;
constexpr ImU32 MAP_TILE_HOVER_FILL = IM_COL32(255, 255, 255, 90);
constexpr ImU32 MAP_TILE_HOVER_BORDER = IM_COL32(255, 255, 255, 230);
constexpr float MAP_MIN_GRID_PIXELS_PER_TILE = 6.0f;
} // namespace

ImU32 getTileColor(RegionId regionId, TerrainId terrainId, int16_t elevation) {
    (void)elevation;
    if (terrainId == TerrainId::Water) {
        return IM_COL32(148, 204, 232, 255);
    }
    if (terrainId == TerrainId::Park) {
        return IM_COL32(84, 160, 76, 255);
    }
    if (terrainId == TerrainId::Plaza) {
        return IM_COL32(204, 200, 190, 255);
    }
    const ImU32 boroughColor = getBoroughColor(regionId);
    if (terrainId == TerrainId::Road) {
        return scaleColor(boroughColor, MAP_ROAD_COLOR_SCALE);
    }
    return boroughColor;
}

void renderMapTiles(
    ImDrawList* drawList,
    const MapCamera& camera,
    const WorldConfig& worldConfig,
    const ChunkStore& chunkStore,
    const ImVec2& canvasOrigin,
    const ImVec2& canvasSize,
    const WorldCoord* hoveredTileCoord) {
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
    const bool drawTileGrid = tileSizePixels >= MAP_MIN_GRID_PIXELS_PER_TILE;
    const bool drawHover = hoveredTileCoord != nullptr;
    const float canvasCenterX = canvasOrigin.x + canvasWidthPixels * 0.5f;
    const float canvasCenterY = canvasOrigin.y + canvasHeightPixels * 0.5f;
    float hoverMinX = 0.0f;
    float hoverMinY = 0.0f;
    float hoverMaxX = 0.0f;
    float hoverMaxY = 0.0f;
    bool hasHoverRect = false;
    for (int32_t tileY = clampedStartY; tileY <= clampedEndY; ++tileY) {
        for (int32_t tileX = clampedStartX; tileX <= clampedEndX; ++tileX) {
            WorldCoord coord{tileX, tileY};
            const TerrainId terrainId = chunkStore.getTerrainAt(coord);
            const RegionId regionId = chunkStore.getRegionAt(coord);
            const int16_t elevation = chunkStore.getElevationAt(coord);
            const ImU32 tileColor = getTileColor(regionId, terrainId, elevation);
            const float screenMinX = canvasCenterX + (static_cast<float>(tileX) - camera.centerWorldX) * tileSizePixels;
            const float screenMinY = canvasCenterY + (static_cast<float>(tileY) - camera.centerWorldY) * tileSizePixels;
            const float screenMaxX = screenMinX + tileSizePixels;
            const float screenMaxY = screenMinY + tileSizePixels;
            if (screenMaxX < canvasOrigin.x || screenMinX > canvasOrigin.x + canvasWidthPixels) {
                continue;
            }
            if (screenMaxY < canvasOrigin.y || screenMinY > canvasOrigin.y + canvasHeightPixels) {
                continue;
            }
            const int pixelMinX = static_cast<int>(std::floor(screenMinX));
            const int pixelMinY = static_cast<int>(std::floor(screenMinY));
            int pixelMaxX = static_cast<int>(std::ceil(screenMaxX));
            int pixelMaxY = static_cast<int>(std::ceil(screenMaxY));
            if (pixelMaxX <= pixelMinX) {
                pixelMaxX = pixelMinX + 1;
            }
            if (pixelMaxY <= pixelMinY) {
                pixelMaxY = pixelMinY + 1;
            }
            const float fillMinX = static_cast<float>(pixelMinX);
            const float fillMinY = static_cast<float>(pixelMinY);
            const float fillMaxX = static_cast<float>(pixelMaxX);
            const float fillMaxY = static_cast<float>(pixelMaxY);
            drawList->AddRectFilled(ImVec2(fillMinX, fillMinY), ImVec2(fillMaxX, fillMaxY), tileColor);
            if (drawTileGrid && terrainId != TerrainId::Water) {
                const ImU32 edgeColor = scaleColor(tileColor, MAP_GRID_EDGE_SCALE);
                drawList->AddRectFilled(ImVec2(fillMinX, fillMinY), ImVec2(fillMaxX, fillMinY + 1.0f), edgeColor);
                drawList->AddRectFilled(ImVec2(fillMinX, fillMinY), ImVec2(fillMinX + 1.0f, fillMaxY), edgeColor);
            }
            if (drawHover && hoveredTileCoord->x == tileX && hoveredTileCoord->y == tileY) {
                hoverMinX = fillMinX;
                hoverMinY = fillMinY;
                hoverMaxX = fillMaxX;
                hoverMaxY = fillMaxY;
                hasHoverRect = true;
            }
        }
    }
    if (hasHoverRect) {
        drawList->AddRectFilled(ImVec2(hoverMinX, hoverMinY), ImVec2(hoverMaxX, hoverMaxY), MAP_TILE_HOVER_FILL);
        drawList->AddRect(ImVec2(hoverMinX, hoverMinY), ImVec2(hoverMaxX, hoverMaxY), MAP_TILE_HOVER_BORDER, 0.0f, 0, 1.5f);
    }
}

} // namespace Core
