#pragma once

#include "core/types.h"
#include "ui/map_camera.h"
#include "ui/map_marker_style.h"
#include "ui/viewport_state.h"
#include "imgui.h"

namespace Core {

constexpr float LANDMARK_MARKER_MIN_ZOOM = 2.0f;
constexpr float LANDMARK_LABEL_MIN_ZOOM = MAP_LABEL_ZOOM_THRESHOLD;
constexpr float LANDMARK_HIT_RADIUS_PIXELS = 10.0f;

int32_t findLandmarkIndexAtScreenPoint(
    const MapCamera& camera,
    float screenX,
    float screenY,
    const ImVec2& canvasOrigin,
    const ImVec2& canvasSize,
    float hitRadiusPixels);

void renderLandmarkOverlays(
    ImDrawList* drawList,
    const MapCamera& camera,
    const ImVec2& canvasOrigin,
    const ImVec2& canvasSize,
    const ViewportPickState& viewportPickState);

} // namespace Core
