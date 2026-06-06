#pragma once

#include "ui/map_camera.h"
#include "ui/map_marker_style.h"
#include "ui/viewport_state.h"
#include "world/chunk_store.h"
#include "world/world_config.h"
#include "imgui.h"

namespace Core {

constexpr float BUSINESS_LABEL_MIN_ZOOM = MAP_LABEL_ZOOM_THRESHOLD;

void renderBusinessNodesOnMap(
    ImDrawList* drawList,
    const MapCamera& camera,
    const ImVec2& canvasOrigin,
    const ImVec2& canvasSize,
    const ViewportPickState& viewportPickState);

int32_t pickBusinessNodeAtScreen(
    float screenX,
    float screenY,
    const MapCamera& camera,
    const ImVec2& canvasOrigin,
    const ImVec2& canvasSize,
    float hitRadiusPixels);

} // namespace Core
