#pragma once

#include "core/types.h"
#include <string_view>

namespace Core {

struct LandmarkRect {
    LandmarkId id;
    RegionId region;
    TerrainId terrain;
    bool manhattanOnly;
    int32_t minX;
    int32_t minY;
    int32_t maxX;
    int32_t maxY;
};

class LandmarkTable {
public:
    static std::string_view getLandmarkName(LandmarkId landmarkId);
    static int32_t getLandmarkCount();
    static const LandmarkRect& getLandmark(int32_t index);
};

} // namespace Core
