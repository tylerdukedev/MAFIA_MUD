#pragma once

#include <cstdint>

namespace Core {

constexpr int32_t OPERATIONS_PANEL_SECTION_COUNT = 8;
constexpr int32_t BUSINESS_PANEL_SECTION_COUNT = 5;
constexpr int32_t CONTACTS_PANEL_SECTION_COUNT = 6;

struct GamePanelVisibility {
    bool showSimulation = true;
    bool showCharacter = true;
    bool showBoroughs = true;
    bool showCity = true;
    bool showTileInspector = true;
    bool showMapViewport = true;
    bool showOperations = true;
    bool showBusiness = true;
    bool showContacts = true;
    int32_t travelTargetX = 240;
    int32_t travelTargetY = 240;
    int32_t travelModeIndex = 0;
    int32_t operationsSectionOrder[OPERATIONS_PANEL_SECTION_COUNT]{0, 1, 2, 3, 4, 5, 6, 7};
    int32_t businessSectionOrder[BUSINESS_PANEL_SECTION_COUNT]{0, 1, 2, 3, 4};
    int32_t contactsSectionOrder[CONTACTS_PANEL_SECTION_COUNT]{0, 1, 2, 3, 4, 5};
};

inline void resetGamePanelVisibility(GamePanelVisibility& visibility) {
    visibility = GamePanelVisibility{};
}

} // namespace Core
