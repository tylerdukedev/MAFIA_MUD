#pragma once

#include "core/types.h"

namespace Core {

constexpr float DEFAULT_MAP_PIXELS_PER_TILE = 4.87f;

struct MapCamera {
    float centerWorldX = 256.0f;
    float centerWorldY = 256.0f;
    float pixelsPerTile = 1.0f;
    void panPixels(float deltaPixelsX, float deltaPixelsY);
    void zoomAt(float zoomFactor, float focusWorldX, float focusWorldY);
    void fitToWorld(int32_t worldWidthTiles, int32_t worldHeightTiles, float canvasWidthPixels, float canvasHeightPixels);
    void centerOnTile(int32_t tileX, int32_t tileY, float targetPixelsPerTile);
    WorldCoord worldToTile(float worldX, float worldY) const;
    void tileToScreen(float worldX, float worldY, float canvasOriginX, float canvasOriginY, float canvasWidthPixels, float canvasHeightPixels, float& outScreenX, float& outScreenY) const;
    void screenToWorld(float screenX, float screenY, float canvasOriginX, float canvasOriginY, float canvasWidthPixels, float canvasHeightPixels, float& outWorldX, float& outWorldY) const;
};

void initializeMapCameraForStartingBorough(MapCamera& camera, int32_t boroughIndex);

} // namespace Core
