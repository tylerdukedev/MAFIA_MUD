#include "ui/business_renderer.h"
#include "ui/map_marker_style.h"
#include "world/business_node_table.h"

namespace Core {

namespace {

void getBusinessScreenCenter(
    const MapCamera& camera,
    const BusinessNodeDefinition& business,
    const ImVec2& canvasOrigin,
    const ImVec2& canvasSize,
    float& outScreenX,
    float& outScreenY) {
    float tileMinX = 0.0f;
    float tileMinY = 0.0f;
    float tileMaxX = 0.0f;
    float tileMaxY = 0.0f;
    camera.tileToScreen(
        static_cast<float>(business.tileX),
        static_cast<float>(business.tileY),
        canvasOrigin.x,
        canvasOrigin.y,
        canvasSize.x,
        canvasSize.y,
        tileMinX,
        tileMinY);
    camera.tileToScreen(
        static_cast<float>(business.tileX + 1),
        static_cast<float>(business.tileY + 1),
        canvasOrigin.x,
        canvasOrigin.y,
        canvasSize.x,
        canvasSize.y,
        tileMaxX,
        tileMaxY);
    outScreenX = (tileMinX + tileMaxX) * 0.5f;
    outScreenY = (tileMinY + tileMaxY) * 0.5f;
}

} // namespace

int32_t pickBusinessNodeAtScreen(
    float screenX,
    float screenY,
    const MapCamera& camera,
    const ImVec2& canvasOrigin,
    const ImVec2& canvasSize,
    float hitRadiusPixels) {
    int32_t closestBusinessIndex = -1;
    float closestDistanceSquared = hitRadiusPixels * hitRadiusPixels;
    const int32_t businessCount = getBusinessNodeCount();
    for (int32_t businessIndex = 0; businessIndex < businessCount; ++businessIndex) {
        const BusinessNodeDefinition* business = getBusinessNodeDefinition(businessIndex);
        if (business == nullptr) {
            continue;
        }
        float centerX = 0.0f;
        float centerY = 0.0f;
        getBusinessScreenCenter(camera, *business, canvasOrigin, canvasSize, centerX, centerY);
        const float deltaX = screenX - centerX;
        const float deltaY = screenY - centerY;
        const float distanceSquared = deltaX * deltaX + deltaY * deltaY;
        if (distanceSquared <= closestDistanceSquared) {
            closestDistanceSquared = distanceSquared;
            closestBusinessIndex = businessIndex;
        }
    }
    return closestBusinessIndex;
}

void renderBusinessNodesOnMap(
    ImDrawList* drawList,
    const MapCamera& camera,
    const ImVec2& canvasOrigin,
    const ImVec2& canvasSize,
    const ViewportPickState& viewportPickState) {
    const int32_t businessCount = getBusinessNodeCount();
    for (int32_t businessIndex = 0; businessIndex < businessCount; ++businessIndex) {
        const BusinessNodeDefinition* business = getBusinessNodeDefinition(businessIndex);
        if (business == nullptr) {
            continue;
        }
        float centerX = 0.0f;
        float centerY = 0.0f;
        getBusinessScreenCenter(camera, *business, canvasOrigin, canvasSize, centerX, centerY);
        const bool isHovered = viewportPickState.hasBusinessHover && viewportPickState.hoveredBusinessIndex == businessIndex;
        const bool isSelected = viewportPickState.hasBusinessSelection && viewportPickState.selectedBusinessIndex == businessIndex;
        const LocationCategory category = getBusinessLocationCategory(*business);
        const ImU32 color = getLocationMarkerColor(category, isHovered, isSelected);
        drawMapMarkerCircle(drawList, centerX, centerY, MAP_MARKER_DEFAULT_RADIUS_PIXELS, color);
        if (camera.pixelsPerTile >= MAP_LABEL_ZOOM_THRESHOLD) {
            drawMapMarkerLabel(drawList, centerX, centerY, MAP_MARKER_DEFAULT_RADIUS_PIXELS, business->mapLabel);
        }
    }
}

} // namespace Core
