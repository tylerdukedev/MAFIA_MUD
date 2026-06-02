#include "ui/context_help.h"
#include "ui/help_manual.h"
#include "imgui.h"
#include <cstdio>
#include <cstring>

namespace Core {

namespace {

void copyString(char* destination, size_t destinationSize, const char* source) {
    if (destination == nullptr || destinationSize == 0) {
        return;
    }
    if (source == nullptr) {
        destination[0] = '\0';
        return;
    }
    std::snprintf(destination, destinationSize, "%s", source);
}

void registerHoverTarget(
    ContextHelpState& state,
    const char* title,
    const char* tooltip,
    const char* manualTopicId,
    const ImVec2& rectMin,
    const ImVec2& rectMax) {
    ImGui::SetCursorScreenPos(rectMin);
    const ImVec2 size(rectMax.x - rectMin.x, rectMax.y - rectMin.y);
    if (size.x <= 0.0f || size.y <= 0.0f) {
        return;
    }
    ImGui::PushID(title);
    ImGui::InvisibleButton("##ctx_help_hit", size);
    if (state.isInspectMode) {
        if (ImGui::IsItemHovered()) {
            ImGui::GetWindowDrawList()->AddRect(rectMin, rectMax, IM_COL32(255, 210, 80, 220), 0.0f, 0, 2.0f);
            state.hasHoveredTarget = true;
            copyString(state.hoveredTitle, sizeof(state.hoveredTitle), title);
            copyString(state.hoveredTooltip, sizeof(state.hoveredTooltip), tooltip);
            copyString(state.hoveredManualTopicId, sizeof(state.hoveredManualTopicId), manualTopicId);
        }
        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            state.showInspectPopup = true;
            copyString(state.popupTitle, sizeof(state.popupTitle), title);
            copyString(state.popupTooltip, sizeof(state.popupTooltip), tooltip);
            copyString(state.popupManualTopicId, sizeof(state.popupManualTopicId), manualTopicId);
        }
    } else if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", tooltip);
    }
    ImGui::PopID();
}

void registerLastItem(ContextHelpState& state, const char* title, const char* tooltip, const char* manualTopicId) {
    const ImVec2 rectMin = ImGui::GetItemRectMin();
    const ImVec2 rectMax = ImGui::GetItemRectMax();
    registerHoverTarget(state, title, tooltip, manualTopicId, rectMin, rectMax);
}

} // namespace

void updateContextHelpMode(ContextHelpState& state) {
    state.isInspectMode = ImGui::GetIO().KeyCtrl;
    state.hasHoveredTarget = false;
}

void renderContextHelpCursorOverlay(const ContextHelpState& state) {
    if (!state.isInspectMode) {
        return;
    }
    ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    const ImVec2 mousePos = ImGui::GetIO().MousePos;
    const ImVec2 badgeCenter(mousePos.x + 14.0f, mousePos.y - 14.0f);
    drawList->AddCircleFilled(badgeCenter, 11.0f, IM_COL32(32, 36, 44, 240));
    drawList->AddCircle(badgeCenter, 11.0f, IM_COL32(255, 210, 80, 255), 0, 2.0f);
    drawList->AddText(ImVec2(badgeCenter.x - 4.0f, badgeCenter.y - 8.0f), IM_COL32(255, 210, 80, 255), "?");
}

void renderContextHelpInspectPopup(ContextHelpState& state, HelpManualState& manualState) {
    if (!state.showInspectPopup) {
        return;
    }
    ImGui::OpenPopup("InspectHelp");
    ImGui::SetNextWindowSize(ImVec2(420.0f, 0.0f), ImGuiCond_FirstUseEver);
    if (ImGui::BeginPopupModal("InspectHelp", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted(state.popupTitle);
        ImGui::Separator();
        ImGui::TextWrapped("%s", state.popupTooltip);
        ImGui::Spacing();
        if (ImGui::Button("Open in Manual", ImVec2(140.0f, 0.0f))) {
            openHelpManualTopic(manualState, state.popupManualTopicId);
            state.showInspectPopup = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Close", ImVec2(100.0f, 0.0f))) {
            state.showInspectPopup = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void contextHelpTextLine(const char* text, const char* tooltip, const char* manualTopicId, ContextHelpState& state) {
    ImGui::TextUnformatted(text);
    registerLastItem(state, text, tooltip, manualTopicId);
}

void contextHelpStatBar(const char* label, float valueNormalized, const char* tooltip, const char* manualTopicId, ContextHelpState& state) {
    ImGui::BeginGroup();
    ImGui::TextUnformatted(label);
    ImGui::SameLine(168.0f);
    char fractionLabel[16];
    std::snprintf(fractionLabel, sizeof(fractionLabel), "%.0f%%", valueNormalized * 100.0f);
    ImGui::ProgressBar(valueNormalized, ImVec2(-1.0f, 0.0f), fractionLabel);
    ImGui::EndGroup();
    registerLastItem(state, label, tooltip, manualTopicId);
}

void contextHelpSectionHeader(const char* title, const char* tooltip, const char* manualTopicId, ContextHelpState& state) {
    ImGui::Separator();
    ImGui::TextUnformatted(title);
    registerLastItem(state, title, tooltip, manualTopicId);
}

void contextHelpPanelTag(const char* panelTitle, const char* tooltip, const char* manualTopicId, ContextHelpState& state) {
    if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_RootWindow)) {
        return;
    }
    const ImVec2 rectMin = ImGui::GetWindowPos();
    const ImVec2 rectMax(rectMin.x + ImGui::GetWindowWidth(), rectMin.y + ImGui::GetFrameHeight());
    registerHoverTarget(state, panelTitle, tooltip, manualTopicId, rectMin, rectMax);
}

void contextHelpWrappedText(const char* text, const char* title, const char* tooltip, const char* manualTopicId, ContextHelpState& state) {
    ImGui::TextWrapped("%s", text);
    registerLastItem(state, title, tooltip, manualTopicId);
}

} // namespace Core
