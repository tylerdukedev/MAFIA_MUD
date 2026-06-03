#include "ui/business_renderer.h"
#include "world/business_node_table.h"

namespace Core {

namespace {

constexpr float BUSINESS_MARKER_SIZE_PIXELS = 4.0f;
constexpr float BUSINESS_LABEL_PADDING_X = 3.0f;
constexpr float BUSINESS_LABEL_PADDING_Y = 1.0f;
constexpr float BUSINESS_LABEL_OFFSET_Y = 2.0f;
constexpr ImU32 BUSINESS_MARKER_COLOR = IM_COL32(70, 130, 255, 240);
constexpr ImU32 BUSINESS_MARKER_OUTLINE_COLOR = IM_COL32(12, 14, 18, 255);
constexpr ImU32 BUSINESS_MARKER_HOVER_COLOR = IM_COL32(110, 175, 255, 255);

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
        drawList->AddCircle(
            ImVec2(centerX, centerY),
            BUSINESS_MARKER_SIZE_PIXELS + 1.0f,
            BUSINESS_MARKER_OUTLINE_COLOR,
            0,
            1.5f);
        const ImVec2 labelSize = ImGui::CalcTextSize(business->mapLabel);
        const ImVec2 labelPos(centerX - labelSize.x * 0.5f, centerY + BUSINESS_MARKER_SIZE_PIXELS + BUSINESS_LABEL_OFFSET_Y);
        const ImVec2 labelMin(labelPos.x - BUSINESS_LABEL_PADDING_X, labelPos.y - BUSINESS_LABEL_PADDING_Y);
        const ImVec2 labelMax(labelPos.x + labelSize.x + BUSINESS_LABEL_PADDING_X, labelPos.y + labelSize.y + BUSINESS_LABEL_PADDING_Y);
        drawList->AddRectFilled(labelMin, labelMax, IM_COL32(16, 18, 24, 210));
        drawList->AddText(labelPos, IM_COL32(180, 210, 255, 245), business->mapLabel);
    }
}

} // namespace Core
