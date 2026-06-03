#pragma once

#include "core/types.h"

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
    bool hasBusinessHover = false;
    bool hasBusinessSelection = false;
    int32_t hoveredBusinessIndex = -1;
    int32_t selectedBusinessIndex = -1;
};

} // namespace Core
