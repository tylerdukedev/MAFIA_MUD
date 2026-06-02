#include "ui/panel_manager.h"
#include "imgui.h"
#include "imgui_internal.h"
#include <cstdio>

namespace Core {

namespace {
constexpr const char* PANEL_NAMES[PANEL_COUNT] = {
    "Simulation",
    "Boroughs",
    "Tile Inspector",
    "Map Viewport",
};

void toggleMaximize(PanelManager& manager, int index) {
    if (manager.maximized == index) {
        manager.maximized = -1;
        manager.restoreDock[index] = true;
        return;
    }
    manager.maximized = index;
}

bool isMouseOverTitleStrip() {
    const ImVec2 windowPos = ImGui::GetWindowPos();
    const ImVec2 windowSize = ImGui::GetWindowSize();
    const float frameHeight = ImGui::GetFrameHeight();
    const ImVec2 mousePos = ImGui::GetIO().MousePos;
    const bool withinX = mousePos.x >= windowPos.x && mousePos.x <= windowPos.x + windowSize.x;
    const bool withinY = mousePos.y >= windowPos.y - frameHeight && mousePos.y <= windowPos.y + frameHeight;
    return withinX && withinY;
}
} // namespace

const char* getPanelName(int index) {
    if (index < 0 || index >= PANEL_COUNT) {
        return "Panel";
    }
    return PANEL_NAMES[index];
}

bool beginPanel(PanelManager& manager, int index, ImGuiWindowFlags flags) {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    if (manager.maximized == index) {
        ImGui::SetNextWindowDockID(0, ImGuiCond_Always);
        ImGui::SetNextWindowPos(viewport->WorkPos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(viewport->WorkSize, ImGuiCond_Always);
        ImGui::SetNextWindowFocus();
        flags |= ImGuiWindowFlags_NoDocking;
    } else if (manager.restoreDock[index]) {
        ImGui::SetNextWindowDockID(manager.savedDockId[index], ImGuiCond_Always);
        manager.restoreDock[index] = false;
    }
    const bool visible = ImGui::Begin(getPanelName(index), &manager.open[index], flags);
    if (!manager.open[index] && manager.maximized == index) {
        manager.maximized = -1;
    }
    if (manager.maximized != index && ImGui::GetWindowDockID() != 0) {
        manager.savedDockId[index] = ImGui::GetWindowDockID();
    }
    const bool overTitle = isMouseOverTitleStrip();
    const bool hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
    if (overTitle && hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
        toggleMaximize(manager, index);
    }
    char popupId[48];
    std::snprintf(popupId, sizeof(popupId), "panel_tab_ctx_%d", index);
    if (overTitle && hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        ImGui::OpenPopup(popupId);
    }
    if (ImGui::BeginPopup(popupId)) {
        if (ImGui::MenuItem(manager.maximized == index ? "Restore" : "Maximize")) {
            toggleMaximize(manager, index);
        }
        if (ImGui::MenuItem("Close Tab")) {
            manager.open[index] = false;
        }
        ImGui::EndPopup();
    }
    return visible;
}

void endPanel() {
    ImGui::End();
}

void renderPanelBodyContextMenu(PanelManager& manager, int index) {
    if (!ImGui::BeginPopupContextWindow("panel_body_ctx", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)) {
        return;
    }
    ImGui::TextDisabled("%s", getPanelName(index));
    ImGui::Separator();
    if (ImGui::MenuItem(manager.maximized == index ? "Restore Window" : "Maximize Window")) {
        toggleMaximize(manager, index);
    }
    if (ImGui::MenuItem("Close Window")) {
        manager.open[index] = false;
    }
    ImGui::Separator();
    ImGui::BeginDisabled();
    ImGui::MenuItem("New Info Window");
    ImGui::MenuItem("New Watch Window");
    ImGui::EndDisabled();
    ImGui::EndPopup();
}

void renderReopenItems(PanelManager& manager) {
    for (int index = 0; index < PANEL_COUNT; ++index) {
        const bool isOpen = manager.open[index];
        if (ImGui::MenuItem(getPanelName(index), nullptr, isOpen)) {
            manager.open[index] = !isOpen;
            if (manager.open[index]) {
                manager.restoreDock[index] = true;
            }
        }
    }
}

} // namespace Core
