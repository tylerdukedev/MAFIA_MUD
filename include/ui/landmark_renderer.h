#pragma once

#include "core/types.h"
#include "ui/map_camera.h"
#include "ui/viewport_state.h"
#include "imgui.h"

namespace Core {

constexpr float LANDMARK_MARKER_MIN_ZOOM = 2.0f;
constexpr float LANDMARK_LABEL_MIN_ZOOM = 4.5f;
constexpr float LANDMARK_HIT_RADIUS_PIXELS = 10.0f;

struct LandmarkSnapState {
    bool isSnapped = false;
    bool isBreakCooldown = false;
    int32_t snappedLandmarkIndex = -1;
    float snapBlend = 0.0f;
    float breakDragPixels = 0.0f;
};

struct LandmarkSnapPick {
    float screenX = 0.0f;
    float screenY = 0.0f;
    int32_t landmarkIndex = -1;
    bool shouldSetMousePos = false;
};

void resetLandmarkSnapState(LandmarkSnapState& state);

LandmarkSnapPick updateLandmarkSnapPick(
    LandmarkSnapState& state,
    float rawScreenX,
    float rawScreenY,
    float mouseDeltaX,
    float mouseDeltaY,
    float deltaSeconds,
    const MapCamera& camera,
    const ImVec2& canvasOrigin,
    const ImVec2& canvasSize);

void applyLandmarkSnapCursor(const LandmarkSnapPick& pick);

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
