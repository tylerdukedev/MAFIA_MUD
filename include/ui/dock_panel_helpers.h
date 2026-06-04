#pragma once

#include "imgui.h"

namespace Core {

inline ImGuiWindowFlags dockPanelWindowFlags() {
    return ImGuiWindowFlags_AlwaysVerticalScrollbar;
}

// Returns false when the dock tab is inactive or clipped; still requires a matching ImGui::End().
inline bool beginDockPanelWindow(const char* title, bool* isOpen, ImGuiWindowFlags extraFlags = 0) {
    return ImGui::Begin(title, isOpen, dockPanelWindowFlags() | extraFlags);
}

} // namespace Core
