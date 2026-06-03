#include "ui/business_renderer.h"
#include "world/business_node_table.h"

namespace Core {

namespace {

constexpr float BUSINESS_MARKER_SIZE_PIXELS = 3.5f;
constexpr ImU32 BUSINESS_MARKER_COLOR = IM_COL32(90, 150, 255, 235);
constexpr ImU32 BUSINESS_MARKER_HOVER_COLOR = IM_COL32(140, 200, 255, 255);

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
        const ImU32 color = isHovered || isSelected ? BUSINESS_MARKER_HOVER_COLOR : BUSINESS_MARKER_COLOR;
        drawList->AddCircleFilled(ImVec2(centerX, centerY), BUSINESS_MARKER_SIZE_PIXELS, color);
        if (isHovered || isSelected) {
            drawList->AddText(ImVec2(centerX + 6.0f, centerY - 6.0f), IM_COL32(180, 210, 255, 245), business->mapLabel);
        }
    }
}

} // namespace Core
