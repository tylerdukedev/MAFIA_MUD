#pragma once

#include "core/types.h"

namespace Core {

struct ViewportPickState {
    WorldCoord hoveredCoord;
    bool hasHover = false;
    bool hasSelection = false;
    WorldCoord selectedCoord;
};

} // namespace Core
