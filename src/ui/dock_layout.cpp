#include "ui/dock_layout.h"
#include "ui/game_dock_panels.h"
#include "imgui.h"
#include "imgui_internal.h"

namespace Core {

namespace {
constexpr const char* DOCKSPACE_WINDOW_NAME = "CapitalViceDockSpace";
constexpr const char* DOCKSPACE_HOST_ID = "CapitalViceDockSpaceHostV8";
constexpr float DOCK_LEFT_WIDTH_FRACTION = 0.20f;
constexpr float DOCK_RIGHT_WIDTH_FRACTION = 0.20f;
constexpr float DOCK_BOTTOM_HEIGHT_FRACTION = 0.26f;
constexpr float DOCK_VIEWPORT_RESIZE_THRESHOLD = 2.0f;
constexpr int32_t DOCK_LAYOUT_WARMUP_FRAMES = 1;

enum class DockLayoutCommand : uint8_t {
    None = 0,
    ApplyDefault = 1,
    Reset = 2,
};

DockLayoutCommand pendingLayoutCommand = DockLayoutCommand::None;
bool isDockLayoutCommitted = false;
int32_t layoutWarmupFramesRemaining = 0;
ImGuiID activeDockspaceId = 0;
ImVec2 cachedDockViewportSize = ImVec2(-1.0f, -1.0f);

bool hasDockViewportSizeChanged(const ImVec2& viewportSize) {
    if (cachedDockViewportSize.x < 0.0f) {
        return false;
    }
    return ImFabs(cachedDockViewportSize.x - viewportSize.x) > DOCK_VIEWPORT_RESIZE_THRESHOLD
        || ImFabs(cachedDockViewportSize.y - viewportSize.y) > DOCK_VIEWPORT_RESIZE_THRESHOLD;
}

void dockWindowToZone(const char* windowTitle, ImGuiID zoneNodeId) {
    if (windowTitle == nullptr || zoneNodeId == 0) {
        return;
    }
    ImGui::DockBuilderDockWindow(windowTitle, zoneNodeId);
}

void buildDefaultDockLayout(ImGuiID dockspaceId, const ImVec2& dockspaceSize) {
    ImGui::DockBuilderRemoveNode(dockspaceId);
    ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspaceId, dockspaceSize);
    ImGuiID dockMainId = dockspaceId;
    ImGuiID dockLeftId = ImGui::DockBuilderSplitNode(dockMainId, ImGuiDir_Left, DOCK_LEFT_WIDTH_FRACTION, nullptr, &dockMainId);
    ImGuiID dockRightId = ImGui::DockBuilderSplitNode(dockMainId, ImGuiDir_Right, DOCK_RIGHT_WIDTH_FRACTION, nullptr, &dockMainId);
    ImGuiID dockBottomId = ImGui::DockBuilderSplitNode(dockMainId, ImGuiDir_Down, DOCK_BOTTOM_HEIGHT_FRACTION, nullptr, &dockMainId);
    for (int32_t panelIndex = 0; panelIndex < GAME_DOCK_PANEL_DEFINITION_COUNT; ++panelIndex) {
        const GameDockPanelDef& panelDef = GAME_DOCK_PANEL_DEFINITIONS[panelIndex];
        ImGuiID targetNodeId = dockMainId;
        if (panelDef.zone == GameDockZone::Left) {
            targetNodeId = dockLeftId;
        } else if (panelDef.zone == GameDockZone::Right) {
            targetNodeId = dockRightId;
        } else if (panelDef.zone == GameDockZone::Bottom) {
            targetNodeId = dockBottomId;
        }
        dockWindowToZone(panelDef.windowTitle, targetNodeId);
    }
    ImGui::DockBuilderFinish(dockspaceId);
}

void syncDockLayoutToViewport(ImGuiID dockspaceId, const ImVec2& viewportSize) {
    ImGuiDockNode* dockNode = ImGui::DockBuilderGetNode(dockspaceId);
    if (dockNode == nullptr) {
        buildDefaultDockLayout(dockspaceId, viewportSize);
        cachedDockViewportSize = viewportSize;
        isDockLayoutCommitted = true;
        return;
    }
    if (!hasDockViewportSizeChanged(viewportSize)) {
        return;
    }
    ImGui::DockBuilderSetNodeSize(dockspaceId, viewportSize);
    cachedDockViewportSize = viewportSize;
}

void queueLayoutCommand(DockLayoutCommand command) {
    pendingLayoutCommand = command;
    isDockLayoutCommitted = false;
    layoutWarmupFramesRemaining = DOCK_LAYOUT_WARMUP_FRAMES;
    cachedDockViewportSize = ImVec2(-1.0f, -1.0f);
}

bool shouldApplyPendingLayout() {
    if (pendingLayoutCommand == DockLayoutCommand::None) {
        return false;
    }
    if (layoutWarmupFramesRemaining > 0) {
        layoutWarmupFramesRemaining -= 1;
        return false;
    }
    return activeDockspaceId != 0;
}

void applyPendingLayout(const ImVec2& viewportSize) {
    if (!shouldApplyPendingLayout()) {
        return;
    }
    buildDefaultDockLayout(activeDockspaceId, viewportSize);
    cachedDockViewportSize = viewportSize;
    isDockLayoutCommitted = true;
    pendingLayoutCommand = DockLayoutCommand::None;
}
} // namespace

void requestDefaultGameDockLayout() {
    queueLayoutCommand(DockLayoutCommand::ApplyDefault);
}

void requestResetGameDockLayout() {
    queueLayoutCommand(DockLayoutCommand::Reset);
}

void requestDefaultDockLayoutOnNextFrame() {
    requestDefaultGameDockLayout();
}

void resetDockLayout() {
    requestResetGameDockLayout();
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
    activeDockspaceId = ImGui::GetID(DOCKSPACE_HOST_ID);
    const ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_PassthruCentralNode;
    const ImVec2 dockSpaceSize = ImGui::GetContentRegionAvail();
    ImGui::DockSpace(activeDockspaceId, dockSpaceSize, dockspaceFlags);
    if (!isDockLayoutCommitted && pendingLayoutCommand == DockLayoutCommand::None) {
        syncDockLayoutToViewport(activeDockspaceId, viewport->WorkSize);
    }
    ImGui::End();
}

void finalizeGameDockLayoutForFrame() {
    if (activeDockspaceId == 0) {
        return;
    }
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    applyPendingLayout(viewport->WorkSize);
}

} // namespace Core
