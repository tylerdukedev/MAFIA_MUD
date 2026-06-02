#pragma once

#include "imgui.h"

namespace Core {

enum PanelIndex {
    PANEL_SIMULATION = 0,
    PANEL_BOROUGHS,
    PANEL_TILE_INSPECTOR,
    PANEL_MAP_VIEWPORT,
    PANEL_COUNT
};

struct PanelManager {
    bool open[PANEL_COUNT] = {true, true, true, true};
    int maximized = -1;
    ImGuiID savedDockId[PANEL_COUNT] = {0, 0, 0, 0};
    bool restoreDock[PANEL_COUNT] = {false, false, false, false};
};

const char* getPanelName(int index);
bool beginPanel(PanelManager& manager, int index, ImGuiWindowFlags flags = 0);
void endPanel();
void renderPanelBodyContextMenu(PanelManager& manager, int index);
void renderReopenItems(PanelManager& manager);

} // namespace Core
