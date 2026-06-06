#include "ui/landmark_renderer.h"
#include "ui/map_marker_style.h"
#include "world/landmark_table.h"

namespace Core {

namespace {

void getLandmarkScreenCenter(
    const MapCamera& camera,
    const LandmarkDefinition& landmark,
    const ImVec2& canvasOrigin,
    const ImVec2& canvasSize,
    float& outScreenX,
    float& outScreenY) {
    float tileMinX = 0.0f;
    float tileMinY = 0.0f;
    float tileMaxX = 0.0f;
    float tileMaxY = 0.0f;
    camera.tileToScreen(
        static_cast<float>(landmark.tileX),
        static_cast<float>(landmark.tileY),
        canvasOrigin.x,
        canvasOrigin.y,
        canvasSize.x,
        canvasSize.y,
        tileMinX,
        tileMinY);
    camera.tileToScreen(
        static_cast<float>(landmark.tileX + 1),
        static_cast<float>(landmark.tileY + 1),
        canvasOrigin.x,
        canvasOrigin.y,
        canvasSize.x,
        canvasSize.y,
        tileMaxX,
        tileMaxY);
    outScreenX = (tileMinX + tileMaxX) * 0.5f;
    outScreenY = (tileMinY + tileMaxY) * 0.5f;
}

bool isPointInRect(float screenX, float screenY, const ImVec2& rectMin, const ImVec2& rectMax) {
    return screenX >= rectMin.x && screenX <= rectMax.x && screenY >= rectMin.y && screenY <= rectMax.y;
}

bool isPointOverLandmark(
    float screenX,
    float screenY,
    float centerX,
    float centerY,
    const LandmarkDefinition& landmark,
    float pixelsPerTile,
    float hitRadiusPixels) {
    const float deltaX = screenX - centerX;
    const float deltaY = screenY - centerY;
    const float hitRadiusSquared = hitRadiusPixels * hitRadiusPixels;
    if (deltaX * deltaX + deltaY * deltaY <= hitRadiusSquared) {
        return true;
    }
    if (pixelsPerTile < MAP_LABEL_ZOOM_THRESHOLD) {
        return false;
    }
    ImVec2 labelMin{};
    ImVec2 labelMax{};
    getMapMarkerLabelBounds(centerX, centerY, MAP_MARKER_DEFAULT_RADIUS_PIXELS, landmark.mapLabel, labelMin, labelMax);
    return isPointInRect(screenX, screenY, labelMin, labelMax);
}

} // namespace

int32_t findLandmarkIndexAtScreenPoint(
    const MapCamera& camera,
    float screenX,
    float screenY,
    const ImVec2& canvasOrigin,
    const ImVec2& canvasSize,
    float hitRadiusPixels) {
    int32_t closestLandmarkIndex = -1;
    float closestDistanceSquared = 0.0f;
    const int32_t landmarkCount = getLandmarkCount();
    for (int32_t landmarkIndex = 0; landmarkIndex < landmarkCount; ++landmarkIndex) {
        const LandmarkDefinition* landmark = getLandmarkDefinition(landmarkIndex);
        if (landmark == nullptr) {
            continue;
        }
        float centerX = 0.0f;
        float centerY = 0.0f;
        getLandmarkScreenCenter(camera, *landmark, canvasOrigin, canvasSize, centerX, centerY);
        if (!isPointOverLandmark(screenX, screenY, centerX, centerY, *landmark, camera.pixelsPerTile, hitRadiusPixels)) {
            continue;
        }
        const float deltaX = screenX - centerX;
        const float deltaY = screenY - centerY;
        const float distanceSquared = deltaX * deltaX + deltaY * deltaY;
        if (closestLandmarkIndex < 0 || distanceSquared < closestDistanceSquared) {
            closestLandmarkIndex = landmarkIndex;
            closestDistanceSquared = distanceSquared;
        }
    }
    return closestLandmarkIndex;
}

void renderLandmarkOverlays(
    ImDrawList* drawList,
    const MapCamera& camera,
    const ImVec2& canvasOrigin,
    const ImVec2& canvasSize,
    const ViewportPickState& viewportPickState) {
    if (camera.pixelsPerTile < LANDMARK_MARKER_MIN_ZOOM) {
        return;
    }
    const int32_t landmarkCount = getLandmarkCount();
    for (int32_t landmarkIndex = 0; landmarkIndex < landmarkCount; ++landmarkIndex) {
        const LandmarkDefinition* landmark = getLandmarkDefinition(landmarkIndex);
        if (landmark == nullptr) {
            continue;
        }
        float centerX = 0.0f;
        float centerY = 0.0f;
        getLandmarkScreenCenter(camera, *landmark, canvasOrigin, canvasSize, centerX, centerY);
        if (centerX < canvasOrigin.x || centerX > canvasOrigin.x + canvasSize.x) {
            continue;
        }
        if (centerY < canvasOrigin.y || centerY > canvasOrigin.y + canvasSize.y) {
            continue;
        }
        const bool isHovered = viewportPickState.hasLandmarkHover && viewportPickState.hoveredLandmarkIndex == landmarkIndex;
        const bool isSelected = viewportPickState.hasLandmarkSelection && viewportPickState.selectedLandmarkIndex == landmarkIndex;
        const ImU32 markerColor = getLocationMarkerColor(LocationCategory::Landmark, isHovered, isSelected);
        drawMapMarkerCircle(drawList, centerX, centerY, MAP_MARKER_DEFAULT_RADIUS_PIXELS, markerColor);
        if (camera.pixelsPerTile >= MAP_LABEL_ZOOM_THRESHOLD) {
            drawMapMarkerLabel(drawList, centerX, centerY, MAP_MARKER_DEFAULT_RADIUS_PIXELS, landmark->mapLabel);
        }
        if (isSelected) {
            WorldCoord coord{landmark->tileX, landmark->tileY};
            float tileMinX = 0.0f;
            float tileMinY = 0.0f;
            float tileMaxX = 0.0f;
            float tileMaxY = 0.0f;
            camera.tileToScreen(
                static_cast<float>(coord.x),
                static_cast<float>(coord.y),
                canvasOrigin.x,
                canvasOrigin.y,
                canvasSize.x,
                canvasSize.y,
                tileMinX,
                tileMinY);
            camera.tileToScreen(
                static_cast<float>(coord.x + 1),
                static_cast<float>(coord.y + 1),
                canvasOrigin.x,
                canvasOrigin.y,
                canvasSize.x,
                canvasSize.y,
                tileMaxX,
                tileMaxY);
            drawList->AddRect(
                ImVec2(tileMinX, tileMinY),
                ImVec2(tileMaxX, tileMaxY),
                getLocationMarkerColor(LocationCategory::Landmark, true, true),
                0.0f,
                0,
                2.0f);
        }
    }
}

} // namespace Core
