#pragma once

#include "core/types.h"
#include <cstdint>

namespace Core {

constexpr int32_t DISTRICT_TILE_SIZE = 16;
constexpr int32_t DISTRICT_COUNT = 32;
constexpr int32_t DISTRICT_CELL_COUNT = DISTRICT_COUNT * DISTRICT_COUNT;

using DistrictId = uint16_t;
constexpr DistrictId INVALID_DISTRICT_ID = UINT16_MAX;

struct DistrictCoord {
    int32_t x = 0;
    int32_t y = 0;
};

struct DistrictGrid {
    static bool isWithinDistrictBounds(const DistrictCoord& coord);
    static DistrictCoord worldToDistrictCoord(const WorldCoord& worldCoord);
    static WorldCoord districtToWorldOrigin(const DistrictCoord& coord);
    static DistrictId districtCoordToId(const DistrictCoord& coord);
    static DistrictCoord districtIdToCoord(DistrictId districtId);
};

} // namespace Core
