#pragma once

#include "imgui.h"

namespace Core {

inline void resetDockPanelScrollIfNeeded() {
    if (ImGui::IsWindowAppearing()) {
        ImGui::SetScrollY(0.0f);
        return;
    }
    const float scrollMaxY = ImGui::GetScrollMaxY();
    const float scrollY = ImGui::GetScrollY();
    if (scrollY > scrollMaxY) {
        ImGui::SetScrollY(scrollMaxY);
    }
}

} // namespace Core
