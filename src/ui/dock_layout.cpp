#include "ui/dock_layout.h"
#include "imgui.h"
#include "imgui_internal.h"

namespace Core {

namespace {
constexpr const char* DOCKSPACE_WINDOW_NAME = "CapitalViceDockSpace";
constexpr const char* DOCKSPACE_HOST_ID = "CapitalViceDockSpaceHost";
constexpr const char* WINDOW_SIMULATION = "Simulation";
constexpr const char* WINDOW_REGIONS = "Regions";
constexpr const char* WINDOW_TILE_INSPECTOR = "Tile Inspector";
constexpr const char* WINDOW_MAP_VIEWPORT = "Map Viewport";

void buildDefaultDockLayout(ImGuiID dockspaceId, const ImVec2& dockspaceSize) {
    ImGui::DockBuilderRemoveNode(dockspaceId);
    ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_None);
    ImGui::DockBuilderSetNodeSize(dockspaceId, dockspaceSize);
    ImGuiID dockMainId = dockspaceId;
    ImGuiID dockLeftId = ImGui::DockBuilderSplitNode(dockMainId, ImGuiDir_Left, 0.22f, nullptr, &dockMainId);
    ImGuiID dockRightId = ImGui::DockBuilderSplitNode(dockMainId, ImGuiDir_Right, 0.24f, nullptr, &dockMainId);
    ImGuiID dockBottomId = ImGui::DockBuilderSplitNode(dockMainId, ImGuiDir_Down, 0.28f, nullptr, &dockMainId);
    ImGui::DockBuilderDockWindow(WINDOW_SIMULATION, dockLeftId);
    ImGui::DockBuilderDockWindow(WINDOW_REGIONS, dockRightId);
    ImGui::DockBuilderDockWindow(WINDOW_TILE_INSPECTOR, dockBottomId);
    ImGui::DockBuilderDockWindow(WINDOW_MAP_VIEWPORT, dockMainId);
    ImGui::DockBuilderFinish(dockspaceId);
}
} // namespace

void setupDefaultDockLayoutIfNeeded() {
}

void beginMainDockSpace() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDocking;
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin(DOCKSPACE_WINDOW_NAME, nullptr, windowFlags);
    ImGui::PopStyleVar(3);
    ImGuiID dockspaceId = ImGui::GetID(DOCKSPACE_HOST_ID);
    ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_PassthruCentralNode;
    ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), dockspaceFlags);
    if (ImGui::DockBuilderGetNode(dockspaceId) == nullptr) {
        buildDefaultDockLayout(dockspaceId, viewport->WorkSize);
    }
    ImGui::End();
}

void resetDockLayout() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGuiID dockspaceId = ImGui::GetID(DOCKSPACE_HOST_ID);
    buildDefaultDockLayout(dockspaceId, viewport->WorkSize);
}

} // namespace Core
