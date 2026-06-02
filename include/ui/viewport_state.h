#pragma once

#include "core/types.h"
#include "world/district_grid.h"

namespace Core {

struct ViewportPickState {
    WorldCoord hoveredCoord;
    bool hasHover = false;
    bool hasSelection = false;
    WorldCoord selectedCoord;
    bool hasLandmarkHover = false;
    bool hasLandmarkSelection = false;
    int32_t hoveredLandmarkIndex = -1;
    int32_t selectedLandmarkIndex = -1;
    bool hasDistrictHover = false;
    bool hasDistrictSelection = false;
    DistrictCoord hoveredDistrictCoord{};
    DistrictCoord selectedDistrictCoord{};
    DistrictId hoveredDistrictId = INVALID_DISTRICT_ID;
    DistrictId selectedDistrictId = INVALID_DISTRICT_ID;
};

} // namespace Core
