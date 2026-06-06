#pragma once

#include "character/character_draft.h"
#include "game/player_world_state.h"
#include "ui/map_camera.h"
#include "ui/map_marker_style.h"
#include "ui/viewport_state.h"
#include "imgui.h"
#include <cstdint>

namespace Core {

constexpr float PLAYER_MARKER_SIZE_PIXELS = 6.0f;
constexpr float PLAYER_LABEL_MIN_ZOOM = MAP_LABEL_ZOOM_THRESHOLD;
constexpr float PLAYER_HIT_RADIUS_PIXELS = 12.0f;

void renderPlayerMapMarker(
    ImDrawList* drawList,
    const MapCamera& camera,
    const CharacterDraft& characterDraft,
    const PlayerWorldState& worldState,
    uint64_t tickCount,
    const ImVec2& canvasOrigin,
    const ImVec2& canvasSize,
    const ViewportPickState& viewportPickState);

bool pickPlayerMarkerAtScreen(
    float screenX,
    float screenY,
    const MapCamera& camera,
    const PlayerWorldState& worldState,
    uint64_t tickCount,
    const ImVec2& canvasOrigin,
    const ImVec2& canvasSize,
    float hitRadiusPixels);

} // namespace Core
