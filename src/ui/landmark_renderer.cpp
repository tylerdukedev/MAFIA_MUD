#include "ui/landmark_renderer.h"
#include "world/landmark_table.h"
#include <cmath>

namespace Core {

namespace {

constexpr float LANDMARK_MARKER_SIZE_PIXELS = 4.0f;
constexpr float LANDMARK_SNAP_CAPTURE_RADIUS = 34.0f;
constexpr float LANDMARK_SNAP_BREAK_DRAG = 14.0f;
constexpr float LANDMARK_SNAP_IN_SPEED = 20.0f;
constexpr float LANDMARK_SNAP_OUT_SPEED = 14.0f;
constexpr float LANDMARK_SNAP_LOCK_BLEND = 0.55f;
constexpr float LANDMARK_SNAP_RELEASE_BLEND = 0.08f;

void getLandmarkScreenCenter(
    const MapCamera& camera,
    const LandmarkDefinition& landmark,
    const ImVec2& canvasOrigin,
    const ImVec2& canvasSize,
    float& outScreenX,
    float& outScreenY) {
    float tileMinX = 0.0f;
    float tileMinY = 0.0f;
    float tileMaxX = 0.0f;
    float tileMaxY = 0.0f;
    camera.tileToScreen(
        static_cast<float>(landmark.tileX),
        static_cast<float>(landmark.tileY),
        canvasOrigin.x,
        canvasOrigin.y,
        canvasSize.x,
        canvasSize.y,
        tileMinX,
        tileMinY);
    camera.tileToScreen(
        static_cast<float>(landmark.tileX + 1),
        static_cast<float>(landmark.tileY + 1),
        canvasOrigin.x,
        canvasOrigin.y,
        canvasSize.x,
        canvasSize.y,
        tileMaxX,
        tileMaxY);
    outScreenX = (tileMinX + tileMaxX) * 0.5f;
    outScreenY = (tileMinY + tileMaxY) * 0.5f;
}

float clampUnit(float value) {
    if (value < 0.0f) {
        return 0.0f;
    }
    if (value > 1.0f) {
        return 1.0f;
    }
    return value;
}

void applySnapBlend(LandmarkSnapState& state, float targetBlend, float deltaSeconds) {
    if (targetBlend > state.snapBlend) {
        state.snapBlend = clampUnit(state.snapBlend + LANDMARK_SNAP_IN_SPEED * deltaSeconds);
        return;
    }
    if (targetBlend < state.snapBlend) {
        state.snapBlend = clampUnit(state.snapBlend - LANDMARK_SNAP_OUT_SPEED * deltaSeconds);
    }
}

} // namespace

void resetLandmarkSnapState(LandmarkSnapState& state) {
    state.isSnapped = false;
    state.isBreakCooldown = false;
    state.snappedLandmarkIndex = -1;
    state.snapBlend = 0.0f;
    state.breakDragPixels = 0.0f;
}

LandmarkSnapPick updateLandmarkSnapPick(
    LandmarkSnapState& state,
    float rawScreenX,
    float rawScreenY,
    float mouseDeltaX,
    float mouseDeltaY,
    float deltaSeconds,
    const MapCamera& camera,
    const ImVec2& canvasOrigin,
    const ImVec2& canvasSize) {
    LandmarkSnapPick pick{};
    pick.screenX = rawScreenX;
    pick.screenY = rawScreenY;
    const int32_t nearestLandmarkIndex = findLandmarkIndexAtScreenPoint(
        camera,
        rawScreenX,
        rawScreenY,
        canvasOrigin,
        canvasSize,
        LANDMARK_SNAP_CAPTURE_RADIUS);
    float targetBlend = 0.0f;
    int32_t activeLandmarkIndex = -1;
    if (state.isSnapped && state.snappedLandmarkIndex >= 0) {
        const LandmarkDefinition* snappedLandmark = getLandmarkDefinition(state.snappedLandmarkIndex);
        if (snappedLandmark != nullptr) {
            float centerX = 0.0f;
            float centerY = 0.0f;
            getLandmarkScreenCenter(camera, *snappedLandmark, canvasOrigin, canvasSize, centerX, centerY);
            const float awayX = rawScreenX - centerX;
            const float awayY = rawScreenY - centerY;
            const float awayLength = std::sqrt(awayX * awayX + awayY * awayY);
            if (awayLength > 0.001f) {
                const float awayNormX = awayX / awayLength;
                const float awayNormY = awayY / awayLength;
                const float awayDelta = mouseDeltaX * awayNormX + mouseDeltaY * awayNormY;
                if (awayDelta > 0.0f) {
                    state.breakDragPixels += awayDelta;
                }
            }
            if (state.breakDragPixels >= LANDMARK_SNAP_BREAK_DRAG) {
                state.isSnapped = false;
                state.snappedLandmarkIndex = -1;
                state.breakDragPixels = 0.0f;
                state.isBreakCooldown = true;
                targetBlend = 0.0f;
            } else {
                targetBlend = 1.0f;
                activeLandmarkIndex = state.snappedLandmarkIndex;
            }
        } else {
            state.isSnapped = false;
            state.snappedLandmarkIndex = -1;
        }
    }
    if (!state.isSnapped) {
        if (state.isBreakCooldown) {
            if (nearestLandmarkIndex < 0) {
                state.isBreakCooldown = false;
            }
        } else if (nearestLandmarkIndex >= 0 && state.snapBlend <= LANDMARK_SNAP_RELEASE_BLEND) {
            state.isSnapped = true;
            state.snappedLandmarkIndex = nearestLandmarkIndex;
            state.breakDragPixels = 0.0f;
            targetBlend = 1.0f;
            activeLandmarkIndex = nearestLandmarkIndex;
        }
    }
    applySnapBlend(state, targetBlend, deltaSeconds);
    if (activeLandmarkIndex >= 0 && state.snapBlend > 0.001f) {
        const LandmarkDefinition* activeLandmark = getLandmarkDefinition(activeLandmarkIndex);
        if (activeLandmark != nullptr) {
            float centerX = 0.0f;
            float centerY = 0.0f;
            getLandmarkScreenCenter(camera, *activeLandmark, canvasOrigin, canvasSize, centerX, centerY);
            pick.screenX = rawScreenX + (centerX - rawScreenX) * state.snapBlend;
            pick.screenY = rawScreenY + (centerY - rawScreenY) * state.snapBlend;
            if (state.snapBlend >= LANDMARK_SNAP_LOCK_BLEND) {
                pick.landmarkIndex = activeLandmarkIndex;
            }
        }
    }
    if (pick.landmarkIndex < 0 && state.snapBlend <= LANDMARK_SNAP_RELEASE_BLEND) {
        pick.landmarkIndex = findLandmarkIndexAtScreenPoint(
            camera,
            pick.screenX,
            pick.screenY,
            canvasOrigin,
            canvasSize,
            LANDMARK_HIT_RADIUS_PIXELS);
    }
    return pick;
}

int32_t findLandmarkIndexAtScreenPoint(
    const MapCamera& camera,
    float screenX,
    float screenY,
    const ImVec2& canvasOrigin,
    const ImVec2& canvasSize,
    float hitRadiusPixels) {
    int32_t closestLandmarkIndex = -1;
    float closestDistanceSquared = hitRadiusPixels * hitRadiusPixels;
    const int32_t landmarkCount = getLandmarkCount();
    for (int32_t landmarkIndex = 0; landmarkIndex < landmarkCount; ++landmarkIndex) {
        const LandmarkDefinition* landmark = getLandmarkDefinition(landmarkIndex);
        if (landmark == nullptr) {
            continue;
        }
        float centerX = 0.0f;
        float centerY = 0.0f;
        getLandmarkScreenCenter(camera, *landmark, canvasOrigin, canvasSize, centerX, centerY);
        const float deltaX = screenX - centerX;
        const float deltaY = screenY - centerY;
        const float distanceSquared = deltaX * deltaX + deltaY * deltaY;
        if (distanceSquared <= closestDistanceSquared) {
            closestDistanceSquared = distanceSquared;
            closestLandmarkIndex = landmarkIndex;
        }
    }
    return closestLandmarkIndex;
}

void renderLandmarkOverlays(
    ImDrawList* drawList,
    const MapCamera& camera,
    const ImVec2& canvasOrigin,
    const ImVec2& canvasSize,
    const ViewportPickState& viewportPickState) {
    if (camera.pixelsPerTile < LANDMARK_MARKER_MIN_ZOOM) {
        return;
    }
    const int32_t landmarkCount = getLandmarkCount();
    for (int32_t landmarkIndex = 0; landmarkIndex < landmarkCount; ++landmarkIndex) {
        const LandmarkDefinition* landmark = getLandmarkDefinition(landmarkIndex);
        if (landmark == nullptr) {
            continue;
        }
        float centerX = 0.0f;
        float centerY = 0.0f;
        getLandmarkScreenCenter(camera, *landmark, canvasOrigin, canvasSize, centerX, centerY);
        if (centerX < canvasOrigin.x || centerX > canvasOrigin.x + canvasSize.x) {
            continue;
        }
        if (centerY < canvasOrigin.y || centerY > canvasOrigin.y + canvasSize.y) {
            continue;
        }
        const bool isHovered = viewportPickState.hasLandmarkHover && viewportPickState.hoveredLandmarkIndex == landmarkIndex;
        const bool isSelected = viewportPickState.hasLandmarkSelection && viewportPickState.selectedLandmarkIndex == landmarkIndex;
        const ImU32 markerColor = isSelected ? IM_COL32(255, 230, 120, 255)
            : (isHovered ? IM_COL32(255, 210, 80, 255) : IM_COL32(240, 244, 252, 230));
        drawList->AddCircleFilled(ImVec2(centerX, centerY), LANDMARK_MARKER_SIZE_PIXELS, markerColor);
        drawList->AddCircle(ImVec2(centerX, centerY), LANDMARK_MARKER_SIZE_PIXELS + 1.0f, IM_COL32(20, 22, 28, 220), 0, 1.5f);
        if (camera.pixelsPerTile >= LANDMARK_LABEL_MIN_ZOOM) {
            const char* labelText = landmark->mapLabel;
            const ImVec2 labelSize = ImGui::CalcTextSize(labelText);
            const ImVec2 labelPos(centerX - labelSize.x * 0.5f, centerY + LANDMARK_MARKER_SIZE_PIXELS + 2.0f);
            const ImVec2 labelBackgroundMin(labelPos.x - 3.0f, labelPos.y - 1.0f);
            const ImVec2 labelBackgroundMax(labelPos.x + labelSize.x + 3.0f, labelPos.y + labelSize.y + 1.0f);
            drawList->AddRectFilled(labelBackgroundMin, labelBackgroundMax, IM_COL32(16, 18, 24, 210));
            drawList->AddText(labelPos, IM_COL32(235, 238, 245, 245), labelText);
        }
        if (isSelected) {
            WorldCoord coord{landmark->tileX, landmark->tileY};
            float tileMinX = 0.0f;
            float tileMinY = 0.0f;
            float tileMaxX = 0.0f;
            float tileMaxY = 0.0f;
            camera.tileToScreen(
                static_cast<float>(coord.x),
                static_cast<float>(coord.y),
                canvasOrigin.x,
                canvasOrigin.y,
                canvasSize.x,
                canvasSize.y,
                tileMinX,
                tileMinY);
            camera.tileToScreen(
                static_cast<float>(coord.x + 1),
                static_cast<float>(coord.y + 1),
                canvasOrigin.x,
                canvasOrigin.y,
                canvasSize.x,
                canvasSize.y,
                tileMaxX,
                tileMaxY);
            drawList->AddRect(
                ImVec2(tileMinX, tileMinY),
                ImVec2(tileMaxX, tileMaxY),
                IM_COL32(255, 210, 80, 240),
                0.0f,
                0,
                2.0f);
        }
    }
}

} // namespace Core
