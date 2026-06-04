#include "ui/npc_map_marker.h"
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

// Generate distinct color for each NPC based on their agent index
void getNpcMarkerColor(int32_t agentIndex, uint8_t& outR, uint8_t& outG, uint8_t& outB) {
    // Use agent index to generate consistent colors
    const uint32_t hash = static_cast<uint32_t>(agentIndex) * 2654435761u;

    // Generate colors with good saturation and brightness
    const float hue = (hash % 360) / 360.0f;
    const float saturation = 0.7f;
    const float value = 0.85f;

    // HSV to RGB conversion
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

        // Skip if not active or not visible
        if (!agent.isActive || !isAgentVisibleOnMap(agent)) {
            continue;
        }

        // Skip if no position set
        if (agent.currentTileX < 0 || agent.currentTileY < 0) {
            continue;
        }

        // Get screen position
        float centerX = 0.0f;
        float centerY = 0.0f;
        const float tileX = static_cast<float>(agent.currentTileX);
        const float tileY = static_cast<float>(agent.currentTileY);
        getNpcScreenCenter(camera, tileX, tileY, canvasOrigin, canvasSize, centerX, centerY);

        // Generate color for this NPC
        uint8_t r = 0;
        uint8_t g = 0;
        uint8_t b = 0;
        getNpcMarkerColor(agentIndex, r, g, b);

        // Draw filled circle with black outline (same style as player)
        const ImU32 fillColor = IM_COL32(r, g, b, 220);
        const ImU32 outlineColor = IM_COL32(12, 14, 18, 255);

        drawList->AddCircleFilled(ImVec2(centerX, centerY), NPC_MARKER_SIZE_PIXELS, fillColor);
        drawList->AddCircle(ImVec2(centerX, centerY), NPC_MARKER_SIZE_PIXELS + 1.5f, outlineColor, 0, 2.0f);

        // Optional: Draw label at high zoom levels
        if (camera.pixelsPerTile >= NPC_LABEL_MIN_ZOOM) {
            const char* displayName = nullptr;
            const char* roleLabel = nullptr;
            if (tryGetAgentDisplayLabels(agentStore, agentIndex, displayName, roleLabel)) {
                const ImVec2 labelSize = ImGui::CalcTextSize(displayName);
                const ImVec2 labelPos(centerX - labelSize.x * 0.5f, centerY + NPC_MARKER_SIZE_PIXELS + 3.0f);
                const ImVec2 labelMin(labelPos.x - 2.0f, labelPos.y - 1.0f);
                const ImVec2 labelMax(labelPos.x + labelSize.x + 2.0f, labelPos.y + labelSize.y + 1.0f);

                drawList->AddRectFilled(labelMin, labelMax, IM_COL32(16, 18, 24, 200));
                drawList->AddText(labelPos, IM_COL32(220, 220, 220, 255), displayName);
            }
        }
    }
}

} // namespace Core
