#include "ui/player_map_marker.h"

namespace Core {

namespace {

void getPlayerScreenCenter(
    const MapCamera& camera,
    float tileX,
    float tileY,
    const ImVec2& canvasOrigin,
    const ImVec2& canvasSize,
    float& outScreenX,
    float& outScreenY) {
    float tileMinX = 0.0f;
    float tileMinY = 0.0f;
    float tileMaxX = 0.0f;
    float tileMaxY = 0.0f;
    camera.tileToScreen(tileX, tileY, canvasOrigin.x, canvasOrigin.y, canvasSize.x, canvasSize.y, tileMinX, tileMinY);
    camera.tileToScreen(tileX + 1.0f, tileY + 1.0f, canvasOrigin.x, canvasOrigin.y, canvasSize.x, canvasSize.y, tileMaxX, tileMaxY);
    outScreenX = (tileMinX + tileMaxX) * 0.5f;
    outScreenY = (tileMinY + tileMaxY) * 0.5f;
}

} // namespace

void renderPlayerMapMarker(
    ImDrawList* drawList,
    const MapCamera& camera,
    const CharacterDraft& characterDraft,
    const PlayerWorldState& worldState,
    uint64_t tickCount,
    const ImVec2& canvasOrigin,
    const ImVec2& canvasSize,
    const ViewportPickState& viewportPickState) {
    float displayTileX = 0.0f;
    float displayTileY = 0.0f;
    getPlayerDisplayTile(worldState, tickCount, displayTileX, displayTileY);
    float centerX = 0.0f;
    float centerY = 0.0f;
    getPlayerScreenCenter(camera, displayTileX - 0.5f, displayTileY - 0.5f, canvasOrigin, canvasSize, centerX, centerY);
    const bool isHovered = viewportPickState.hasPlayerHover;
    const ImU32 fillColor = IM_COL32(characterDraft.mapMarkerColorR, characterDraft.mapMarkerColorG, characterDraft.mapMarkerColorB, isHovered ? 255 : 235);
    const ImU32 outlineColor = IM_COL32(12, 14, 18, 255);
    drawList->AddCircleFilled(ImVec2(centerX, centerY), PLAYER_MARKER_SIZE_PIXELS, fillColor);
    drawList->AddCircle(ImVec2(centerX, centerY), PLAYER_MARKER_SIZE_PIXELS + 1.5f, outlineColor, 0, 2.0f);
    if (worldState.isTraveling) {
        drawList->AddCircle(ImVec2(centerX, centerY), PLAYER_MARKER_SIZE_PIXELS + 4.0f, IM_COL32(220, 220, 220, 120), 0, 1.0f);
    }
    if (camera.pixelsPerTile >= PLAYER_LABEL_MIN_ZOOM) {
        const char* label = characterDraft.nameBuffer[0] != '\0' ? characterDraft.nameBuffer : "You";
        const ImVec2 labelSize = ImGui::CalcTextSize(label);
        const ImVec2 labelPos(centerX - labelSize.x * 0.5f, centerY + PLAYER_MARKER_SIZE_PIXELS + 3.0f);
        const ImVec2 labelMin(labelPos.x - 3.0f, labelPos.y - 1.0f);
        const ImVec2 labelMax(labelPos.x + labelSize.x + 3.0f, labelPos.y + labelSize.y + 1.0f);
        drawList->AddRectFilled(labelMin, labelMax, IM_COL32(16, 18, 24, 220));
        drawList->AddText(labelPos, IM_COL32(245, 245, 245, 255), label);
    }
}

bool pickPlayerMarkerAtScreen(
    float screenX,
    float screenY,
    const MapCamera& camera,
    const PlayerWorldState& worldState,
    uint64_t tickCount,
    const ImVec2& canvasOrigin,
    const ImVec2& canvasSize,
    float hitRadiusPixels) {
    float displayTileX = 0.0f;
    float displayTileY = 0.0f;
    getPlayerDisplayTile(worldState, tickCount, displayTileX, displayTileY);
    float centerX = 0.0f;
    float centerY = 0.0f;
    getPlayerScreenCenter(camera, displayTileX - 0.5f, displayTileY - 0.5f, canvasOrigin, canvasSize, centerX, centerY);
    const float deltaX = screenX - centerX;
    const float deltaY = screenY - centerY;
    const float hitRadiusSquared = hitRadiusPixels * hitRadiusPixels;
    return (deltaX * deltaX + deltaY * deltaY) <= hitRadiusSquared;
}

} // namespace Core
