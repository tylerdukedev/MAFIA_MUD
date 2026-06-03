#pragma once

#include "game/player_information_feed.h"
#include <cstdint>

namespace Core {

constexpr int32_t MAX_MAP_TOP_TOASTS = 4;
constexpr int32_t MAX_MAP_BOTTOM_EVENTS = 6;
constexpr float MAP_TOAST_DURATION_SECONDS = 3.5f;
constexpr float MAP_EVENT_DURATION_SECONDS = 8.0f;

enum class MapToastKind : uint8_t {
    System = 0,
    Save = 1,
    Event = 2,
};

struct MapToastSlot {
    char text[96]{};
    MapToastKind kind = MapToastKind::System;
    float remainingSeconds = 0.0f;
};

struct MapEventNotificationSlot {
    int32_t feedItemIndex = -1;
    float remainingSeconds = 0.0f;
};

struct MapNotificationLayerState {
    MapToastSlot topToasts[MAX_MAP_TOP_TOASTS]{};
    MapEventNotificationSlot bottomEvents[MAX_MAP_BOTTOM_EVENTS]{};
};

void resetMapNotificationLayerState(MapNotificationLayerState& state);
void tickMapNotificationLayer(MapNotificationLayerState& state, float deltaSeconds);
void pushMapTopToast(MapNotificationLayerState& state, MapToastKind kind, const char* text);
void pushMapEventFromFeed(MapNotificationLayerState& state, const PlayerInformationFeedStore& feedStore, int32_t feedItemIndex);
void renderMapNotificationLayer(
    MapNotificationLayerState& state,
    PlayerInformationFeedStore& feedStore,
    float canvasPosX,
    float canvasPosY,
    float canvasWidth,
    float canvasHeight,
    bool& outPauseSimulation);

} // namespace Core
