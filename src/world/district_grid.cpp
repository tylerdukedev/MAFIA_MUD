#include "world/district_grid.h"
#include "world/world_config.h"

namespace Core {

bool DistrictGrid::isWithinDistrictBounds(const DistrictCoord& coord) {
    return coord.x >= 0 && coord.x < DISTRICT_COUNT && coord.y >= 0 && coord.y < DISTRICT_COUNT;
}

DistrictCoord DistrictGrid::worldToDistrictCoord(const WorldCoord& worldCoord) {
    DistrictCoord outputCoord{};
    if (worldCoord.x < 0) {
        outputCoord.x = 0;
    } else if (worldCoord.x >= WorldConfig::WORLD_WIDTH_TILES) {
        outputCoord.x = DISTRICT_COUNT - 1;
    } else {
        outputCoord.x = worldCoord.x / DISTRICT_TILE_SIZE;
    }
    if (worldCoord.y < 0) {
        outputCoord.y = 0;
    } else if (worldCoord.y >= WorldConfig::WORLD_HEIGHT_TILES) {
        outputCoord.y = DISTRICT_COUNT - 1;
    } else {
        outputCoord.y = worldCoord.y / DISTRICT_TILE_SIZE;
    }
    return outputCoord;
}

WorldCoord DistrictGrid::districtToWorldOrigin(const DistrictCoord& coord) {
    return WorldCoord{coord.x * DISTRICT_TILE_SIZE, coord.y * DISTRICT_TILE_SIZE};
}

DistrictId DistrictGrid::districtCoordToId(const DistrictCoord& coord) {
    if (!isWithinDistrictBounds(coord)) {
        return INVALID_DISTRICT_ID;
    }
    return static_cast<DistrictId>(coord.y * DISTRICT_COUNT + coord.x);
}

DistrictCoord DistrictGrid::districtIdToCoord(DistrictId districtId) {
    DistrictCoord outputCoord{};
    if (districtId >= DISTRICT_CELL_COUNT) {
        return outputCoord;
    }
    outputCoord.x = static_cast<int32_t>(districtId % DISTRICT_COUNT);
    outputCoord.y = static_cast<int32_t>(districtId / DISTRICT_COUNT);
    return outputCoord;
}

} // namespace Core
