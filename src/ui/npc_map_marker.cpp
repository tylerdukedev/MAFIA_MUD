#include "ui/npc_map_marker.h"
#include "ui/map_marker_style.h"
#include "sim/character_agent.h"
#include <cmath>

namespace Core {

namespace {

void getNpcScreenCenter(
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

void getNpcMarkerColor(int32_t agentIndex, uint8_t& outR, uint8_t& outG, uint8_t& outB) {
    const uint32_t hash = static_cast<uint32_t>(agentIndex) * 2654435761u;
    const float hue = (hash % 360) / 360.0f;
    const float saturation = 0.7f;
    const float value = 0.85f;
    const float c = value * saturation;
    const float hueX6 = hue * 6.0f;
    const float x = c * (1.0f - std::fabs(std::fmod(hueX6, 2.0f) - 1.0f));
    const float m = value - c;
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    if (hue < 1.0f / 6.0f) {
        r = c; g = x; b = 0.0f;
    } else if (hue < 2.0f / 6.0f) {
        r = x; g = c; b = 0.0f;
    } else if (hue < 3.0f / 6.0f) {
        r = 0.0f; g = c; b = x;
    } else if (hue < 4.0f / 6.0f) {
        r = 0.0f; g = x; b = c;
    } else if (hue < 5.0f / 6.0f) {
        r = x; g = 0.0f; b = c;
    } else {
        r = c; g = 0.0f; b = x;
    }
    outR = static_cast<uint8_t>((r + m) * 255.0f);
    outG = static_cast<uint8_t>((g + m) * 255.0f);
    outB = static_cast<uint8_t>((b + m) * 255.0f);
}

} // namespace

void renderNpcMapMarkers(
    ImDrawList* drawList,
    const MapCamera& camera,
    const CharacterAgentStore& agentStore,
    const ImVec2& canvasOrigin,
    const ImVec2& canvasSize) {
    for (int32_t agentIndex = 0; agentIndex < MAX_CHARACTER_AGENT_COUNT; ++agentIndex) {
        const CharacterAgentState& agent = agentStore.states[agentIndex];
        if (!agent.isActive || !isAgentVisibleOnMap(agent)) {
            continue;
        }
        if (agent.currentTileX < 0 || agent.currentTileY < 0) {
            continue;
        }
        float centerX = 0.0f;
        float centerY = 0.0f;
        const float tileX = static_cast<float>(agent.currentTileX);
        const float tileY = static_cast<float>(agent.currentTileY);
        getNpcScreenCenter(camera, tileX, tileY, canvasOrigin, canvasSize, centerX, centerY);
        uint8_t r = 0;
        uint8_t g = 0;
        uint8_t b = 0;
        getNpcMarkerColor(agentIndex, r, g, b);
        const ImU32 fillColor = IM_COL32(r, g, b, 220);
        drawMapMarkerCircle(drawList, centerX, centerY, NPC_MARKER_SIZE_PIXELS, fillColor, 2.0f);
        if (camera.pixelsPerTile >= MAP_LABEL_ZOOM_THRESHOLD) {
            const char* displayName = nullptr;
            const char* roleLabel = nullptr;
            if (tryGetAgentDisplayLabels(agentStore, agentIndex, displayName, roleLabel)) {
                drawMapMarkerLabel(drawList, centerX, centerY, NPC_MARKER_SIZE_PIXELS, displayName);
            }
        }
    }
}

} // namespace Core
