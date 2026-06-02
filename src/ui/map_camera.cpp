#include "ui/map_camera.h"
#include <algorithm>
#include <cmath>

namespace Core {

namespace {
constexpr float MIN_PIXELS_PER_TILE = 0.25f;
constexpr float MAX_PIXELS_PER_TILE = 48.0f;
} // namespace

void MapCamera::panPixels(float deltaPixelsX, float deltaPixelsY) {
    centerWorldX -= deltaPixelsX / pixelsPerTile;
    centerWorldY -= deltaPixelsY / pixelsPerTile;
}

void MapCamera::zoomAt(float zoomFactor, float focusWorldX, float focusWorldY) {
    const float newPixelsPerTile = std::clamp(pixelsPerTile * zoomFactor, MIN_PIXELS_PER_TILE, MAX_PIXELS_PER_TILE);
    const float zoomRatio = newPixelsPerTile / pixelsPerTile;
    centerWorldX = focusWorldX + (centerWorldX - focusWorldX) * zoomRatio;
    centerWorldY = focusWorldY + (centerWorldY - focusWorldY) * zoomRatio;
    pixelsPerTile = newPixelsPerTile;
}

void MapCamera::fitToWorld(int32_t worldWidthTiles, int32_t worldHeightTiles, float canvasWidthPixels, float canvasHeightPixels) {
    if (canvasWidthPixels <= 1.0f || canvasHeightPixels <= 1.0f) {
        return;
    }
    const float scaleX = canvasWidthPixels / static_cast<float>(worldWidthTiles);
    const float scaleY = canvasHeightPixels / static_cast<float>(worldHeightTiles);
    pixelsPerTile = std::max(scaleX, scaleY);
    centerWorldX = static_cast<float>(worldWidthTiles) * 0.5f;
    centerWorldY = static_cast<float>(worldHeightTiles) * 0.5f;
}

void MapCamera::centerOnTile(int32_t tileX, int32_t tileY, float targetPixelsPerTile) {
    centerWorldX = static_cast<float>(tileX) + 0.5f;
    centerWorldY = static_cast<float>(tileY) + 0.5f;
    pixelsPerTile = std::clamp(targetPixelsPerTile, MIN_PIXELS_PER_TILE, MAX_PIXELS_PER_TILE);
}

void initializeMapCameraForStartingBorough(MapCamera& camera, int32_t boroughIndex) {
    int32_t tileX = 243;
    int32_t tileY = 208;
    switch (boroughIndex) {
    case 0:
        tileX = 243;
        tileY = 208;
        break;
    case 1:
        tileX = 268;
        tileY = 350;
        break;
    case 2:
        tileX = 384;
        tileY = 230;
        break;
    case 3:
        tileX = 368;
        tileY = 93;
        break;
    case 4:
        tileX = 102;
        tileY = 382;
        break;
    default:
        break;
    }
    camera.centerOnTile(tileX, tileY, DEFAULT_MAP_PIXELS_PER_TILE);
}

WorldCoord MapCamera::worldToTile(float worldX, float worldY) const {
    WorldCoord coord;
    coord.x = static_cast<int32_t>(std::floor(worldX));
    coord.y = static_cast<int32_t>(std::floor(worldY));
    return coord;
}

void MapCamera::tileToScreen(float worldX, float worldY, float canvasOriginX, float canvasOriginY, float canvasWidthPixels, float canvasHeightPixels, float& outScreenX, float& outScreenY) const {
    const float canvasCenterX = canvasOriginX + canvasWidthPixels * 0.5f;
    const float canvasCenterY = canvasOriginY + canvasHeightPixels * 0.5f;
    outScreenX = canvasCenterX + (worldX - centerWorldX) * pixelsPerTile;
    outScreenY = canvasCenterY + (worldY - centerWorldY) * pixelsPerTile;
}

void MapCamera::screenToWorld(float screenX, float screenY, float canvasOriginX, float canvasOriginY, float canvasWidthPixels, float canvasHeightPixels, float& outWorldX, float& outWorldY) const {
    const float canvasCenterX = canvasOriginX + canvasWidthPixels * 0.5f;
    const float canvasCenterY = canvasOriginY + canvasHeightPixels * 0.5f;
    outWorldX = centerWorldX + (screenX - canvasCenterX) / pixelsPerTile;
    outWorldY = centerWorldY + (screenY - canvasCenterY) / pixelsPerTile;
}

} // namespace Core
