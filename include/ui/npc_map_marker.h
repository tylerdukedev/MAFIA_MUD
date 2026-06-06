#pragma once

#include "sim/character_agent.h"
#include "ui/map_camera.h"
#include "ui/map_marker_style.h"
#include "imgui.h"
#include <cstdint>

namespace Core {

constexpr float NPC_MARKER_SIZE_PIXELS = 5.0f;
constexpr float NPC_LABEL_MIN_ZOOM = MAP_LABEL_ZOOM_THRESHOLD;

// Render all visible NPCs on the map
void renderNpcMapMarkers(
    ImDrawList* drawList,
    const MapCamera& camera,
    const CharacterAgentStore& agentStore,
    const ImVec2& canvasOrigin,
    const ImVec2& canvasSize);

} // namespace Core
