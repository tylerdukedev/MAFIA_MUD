#include "ui/dock_layout.h"
#include "imgui.h"
#include "imgui_internal.h"

namespace Core {

namespace {
constexpr const char* DOCKSPACE_WINDOW_NAME = "CapitalViceDockSpace";
constexpr const char* DOCKSPACE_HOST_ID = "CapitalViceDockSpaceHostV6";
constexpr const char* WINDOW_SIMULATION = "Simulation";
constexpr const char* WINDOW_CHARACTER = "Character";
constexpr const char* WINDOW_BOROUGHS = "Boroughs";
constexpr const char* WINDOW_CITY = "City";
constexpr const char* WINDOW_TILE_INSPECTOR = "Tile Inspector";
constexpr const char* WINDOW_MAP_VIEWPORT = "Map Viewport";
constexpr const char* WINDOW_OPERATIONS = "Operations";
constexpr const char* WINDOW_BUSINESS = "Business";
constexpr const char* WINDOW_CONTACTS = "Contacts";
constexpr float DOCK_LEFT_WIDTH_FRACTION = 0.20f;
constexpr float DOCK_RIGHT_WIDTH_FRACTION = 0.20f;
constexpr float DOCK_BOTTOM_HEIGHT_FRACTION = 0.26f;
constexpr float DOCK_VIEWPORT_RESIZE_THRESHOLD = 2.0f;

bool isDefaultDockLayoutPending = false;
ImVec2 cachedDockViewportSize = ImVec2(-1.0f, -1.0f);

bool hasDockViewportSizeChanged(const ImVec2& viewportSize) {
    if (cachedDockViewportSize.x < 0.0f) {
        return false;
    }
    return ImFabs(cachedDockViewportSize.x - viewportSize.x) > DOCK_VIEWPORT_RESIZE_THRESHOLD
        || ImFabs(cachedDockViewportSize.y - viewportSize.y) > DOCK_VIEWPORT_RESIZE_THRESHOLD;
}

void buildDefaultDockLayout(ImGuiID dockspaceId, const ImVec2& dockspaceSize) {
    ImGui::DockBuilderRemoveNode(dockspaceId);
    ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspaceId, dockspaceSize);
    ImGuiID dockMainId = dockspaceId;
    ImGuiID dockLeftId = ImGui::DockBuilderSplitNode(dockMainId, ImGuiDir_Left, DOCK_LEFT_WIDTH_FRACTION, nullptr, &dockMainId);
    ImGuiID dockRightId = ImGui::DockBuilderSplitNode(dockMainId, ImGuiDir_Right, DOCK_RIGHT_WIDTH_FRACTION, nullptr, &dockMainId);
    ImGuiID dockBottomId = ImGui::DockBuilderSplitNode(dockMainId, ImGuiDir_Down, DOCK_BOTTOM_HEIGHT_FRACTION, nullptr, &dockMainId);
    ImGui::DockBuilderDockWindow(WINDOW_TILE_INSPECTOR, dockLeftId);
    ImGui::DockBuilderDockWindow(WINDOW_SIMULATION, dockLeftId);
    ImGui::DockBuilderDockWindow(WINDOW_CHARACTER, dockLeftId);
    ImGui::DockBuilderDockWindow(WINDOW_OPERATIONS, dockLeftId);
    ImGui::DockBuilderDockWindow(WINDOW_BOROUGHS, dockRightId);
    ImGui::DockBuilderDockWindow(WINDOW_CONTACTS, dockRightId);
    ImGui::DockBuilderDockWindow(WINDOW_CITY, dockBottomId);
    ImGui::DockBuilderDockWindow(WINDOW_BUSINESS, dockBottomId);
    ImGui::DockBuilderDockWindow(WINDOW_MAP_VIEWPORT, dockMainId);
    ImGui::DockBuilderFinish(dockspaceId);
}

void syncDockLayoutToViewport(ImGuiID dockspaceId, const ImVec2& viewportSize) {
    ImGuiDockNode* dockNode = ImGui::DockBuilderGetNode(dockspaceId);
    if (dockNode == nullptr) {
        buildDefaultDockLayout(dockspaceId, viewportSize);
        cachedDockViewportSize = viewportSize;
        return;
    }
    if (!hasDockViewportSizeChanged(viewportSize)) {
        return;
    }
    ImGui::DockBuilderSetNodeSize(dockspaceId, viewportSize);
    cachedDockViewportSize = viewportSize;
}
} // namespace

void requestDefaultDockLayoutOnNextFrame() {
    isDefaultDockLayoutPending = true;
}

void setupDefaultDockLayoutIfNeeded() {
    if (!isDefaultDockLayoutPending) {
        return;
    }
    isDefaultDockLayoutPending = false;
    resetDockLayout();
}

void beginMainDockSpace() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDocking;
    ImGui::SetNextWindowPos(viewport->WorkPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(viewport->WorkSize, ImGuiCond_Always);
    ImGui::SetNextWindowViewport(viewport->ID);
    windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoSavedSettings;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin(DOCKSPACE_WINDOW_NAME, nullptr, windowFlags);
    ImGui::PopStyleVar(3);
    ImGuiID dockspaceId = ImGui::GetID(DOCKSPACE_HOST_ID);
    syncDockLayoutToViewport(dockspaceId, viewport->WorkSize);
    ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_PassthruCentralNode;
    const ImVec2 dockSpaceSize = ImGui::GetContentRegionAvail();
    ImGui::DockSpace(dockspaceId, dockSpaceSize, dockspaceFlags);
    ImGui::End();
}

void resetDockLayout() {
    cachedDockViewportSize = ImVec2(-1.0f, -1.0f);
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGuiID dockspaceId = ImGui::GetID(DOCKSPACE_HOST_ID);
    buildDefaultDockLayout(dockspaceId, viewport->WorkSize);
    cachedDockViewportSize = viewport->WorkSize;
}

} // namespace Core
