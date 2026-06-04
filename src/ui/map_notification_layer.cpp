#include "ui/map_notification_layer.h"
#include "imgui.h"
#include <cstdio>
#include <cstring>

namespace Core {

namespace {

void shiftTopToasts(MapNotificationLayerState& state) {
    for (int32_t index = 1; index < MAX_MAP_TOP_TOASTS; ++index) {
        state.topToasts[index - 1] = state.topToasts[index];
    }
    state.topToasts[MAX_MAP_TOP_TOASTS - 1] = MapToastSlot{};
}

const char* informationChannelLabel(InformationChannel channel) {
    switch (channel) {
    case InformationChannel::Newspaper:
        return "Newspaper";
    case InformationChannel::Rumor:
        return "Rumor";
    case InformationChannel::Intel:
        return "Intel";
    default:
        return "News";
    }
}

} // namespace

void resetMapNotificationLayerState(MapNotificationLayerState& state) {
    state = MapNotificationLayerState{};
}

void tickMapNotificationLayer(MapNotificationLayerState& state, float deltaSeconds) {
    for (int32_t toastIndex = 0; toastIndex < MAX_MAP_TOP_TOASTS; ++toastIndex) {
        if (state.topToasts[toastIndex].remainingSeconds > 0.0f) {
            state.topToasts[toastIndex].remainingSeconds -= deltaSeconds;
            if (state.topToasts[toastIndex].remainingSeconds <= 0.0f) {
                state.topToasts[toastIndex] = MapToastSlot{};
            }
        }
    }
    for (int32_t eventIndex = 0; eventIndex < MAX_MAP_BOTTOM_EVENTS; ++eventIndex) {
        if (state.bottomEvents[eventIndex].remainingSeconds > 0.0f) {
            state.bottomEvents[eventIndex].remainingSeconds -= deltaSeconds;
            if (state.bottomEvents[eventIndex].remainingSeconds <= 0.0f) {
                state.bottomEvents[eventIndex] = MapEventNotificationSlot{};
            }
        }
    }
}

void pushMapTopToast(MapNotificationLayerState& state, MapToastKind kind, const char* text) {
    if (text == nullptr || text[0] == '\0') {
        return;
    }
    shiftTopToasts(state);
    MapToastSlot& slot = state.topToasts[MAX_MAP_TOP_TOASTS - 1];
    std::snprintf(slot.text, sizeof(slot.text), "%s", text);
    slot.kind = kind;
    slot.remainingSeconds = MAP_TOAST_DURATION_SECONDS;
}

void pushMapEventFromFeed(MapNotificationLayerState& state, const PlayerInformationFeedStore& feedStore, int32_t feedItemIndex) {
    if (feedItemIndex < 0 || feedItemIndex >= feedStore.itemCount) {
        return;
    }
    for (int32_t eventIndex = 0; eventIndex < MAX_MAP_BOTTOM_EVENTS; ++eventIndex) {
        if (state.bottomEvents[eventIndex].feedItemIndex == feedItemIndex) {
            state.bottomEvents[eventIndex].remainingSeconds = MAP_EVENT_DURATION_SECONDS;
            return;
        }
    }
    for (int32_t shiftIndex = 1; shiftIndex < MAX_MAP_BOTTOM_EVENTS; ++shiftIndex) {
        state.bottomEvents[shiftIndex - 1] = state.bottomEvents[shiftIndex];
    }
    MapEventNotificationSlot& slot = state.bottomEvents[MAX_MAP_BOTTOM_EVENTS - 1];
    slot.feedItemIndex = feedItemIndex;
    slot.remainingSeconds = MAP_EVENT_DURATION_SECONDS;
}

void renderMapNotificationLayer(
    MapNotificationLayerState& state,
    PlayerInformationFeedStore& feedStore,
    float canvasPosX,
    float canvasPosY,
    float canvasWidth,
    float canvasHeight,
    int32_t& outClickedFeedItemIndex) {
    outClickedFeedItemIndex = -1;
    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    const ImVec2 clipMin(canvasPosX, canvasPosY);
    const ImVec2 clipMax(canvasPosX + canvasWidth, canvasPosY + canvasHeight);
    drawList->PushClipRect(clipMin, clipMax, true);
    float topYOffset = 8.0f;
    for (int32_t toastIndex = MAX_MAP_TOP_TOASTS - 1; toastIndex >= 0; --toastIndex) {
        const MapToastSlot& toast = state.topToasts[toastIndex];
        if (toast.remainingSeconds <= 0.0f || toast.text[0] == '\0') {
            continue;
        }
        const ImVec2 textSize = ImGui::CalcTextSize(toast.text);
        const float panelWidth = textSize.x + 24.0f;
        const float panelHeight = textSize.y + 14.0f;
        const ImVec2 panelMin(canvasPosX + (canvasWidth - panelWidth) * 0.5f, canvasPosY + topYOffset);
        const ImVec2 panelMax(panelMin.x + panelWidth, panelMin.y + panelHeight);
        drawList->AddRectFilled(panelMin, panelMax, IM_COL32(18, 20, 28, 230), 4.0f);
        drawList->AddRect(panelMin, panelMax, IM_COL32(90, 100, 120, 255), 4.0f);
        drawList->AddText(ImVec2(panelMin.x + 12.0f, panelMin.y + 7.0f), IM_COL32(235, 238, 245, 255), toast.text);
        topYOffset += panelHeight + 6.0f;
    }
    float bottomYOffset = 8.0f;
    for (int32_t eventIndex = MAX_MAP_BOTTOM_EVENTS - 1; eventIndex >= 0; --eventIndex) {
        const MapEventNotificationSlot& eventSlot = state.bottomEvents[eventIndex];
        if (eventSlot.remainingSeconds <= 0.0f || eventSlot.feedItemIndex < 0) {
            continue;
        }
        if (eventSlot.feedItemIndex >= feedStore.itemCount) {
            continue;
        }
        InformationFeedItem& feedItem = feedStore.items[eventSlot.feedItemIndex];
        char eventLine[160];
        std::snprintf(
            eventLine,
            sizeof(eventLine),
            "[%s] %s",
            informationChannelLabel(feedItem.channel),
            feedItem.headline);
        const ImVec2 textSize = ImGui::CalcTextSize(eventLine);
        const float panelWidth = textSize.x + 28.0f;
        const float panelHeight = textSize.y + 16.0f;
        const ImVec2 panelMax(canvasPosX + canvasWidth - 8.0f, canvasPosY + canvasHeight - bottomYOffset);
        const ImVec2 panelMin(panelMax.x - panelWidth, panelMax.y - panelHeight);
        const ImU32 channelColor = feedItem.channel == InformationChannel::Newspaper
            ? IM_COL32(210, 190, 120, 255)
            : (feedItem.channel == InformationChannel::Intel ? IM_COL32(120, 190, 255, 255) : IM_COL32(200, 160, 200, 255));
        drawList->AddRectFilled(panelMin, panelMax, IM_COL32(16, 18, 26, 235), 4.0f);
        drawList->AddRect(panelMin, panelMax, channelColor, 4.0f);
        drawList->AddText(ImVec2(panelMin.x + 10.0f, panelMin.y + 6.0f), IM_COL32(240, 242, 248, 255), eventLine);
        const ImVec2 dismissPos(panelMax.x - 18.0f, panelMin.y + 4.0f);
        const ImVec2 dismissSize(14.0f, 14.0f);
        const ImVec2 mousePos = ImGui::GetMousePos();
        const bool dismissHovered = mousePos.x >= dismissPos.x && mousePos.x <= dismissPos.x + dismissSize.x
            && mousePos.y >= dismissPos.y && mousePos.y <= dismissPos.y + dismissSize.y;
        drawList->AddText(dismissPos, IM_COL32(200, 120, 120, 255), "x");
        if (dismissHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            feedItem.isRead = true;
            feedItem.isDismissed = true;
            state.bottomEvents[eventIndex] = MapEventNotificationSlot{};
            continue;
        }
        const bool panelHovered = mousePos.x >= panelMin.x && mousePos.x <= panelMax.x && mousePos.y >= panelMin.y && mousePos.y <= panelMax.y;
        if (panelHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !dismissHovered) {
            feedItem.isRead = true;
            outClickedFeedItemIndex = eventSlot.feedItemIndex;
        }
        bottomYOffset += panelHeight + 6.0f;
    }
    drawList->PopClipRect();
}

} // namespace Core
