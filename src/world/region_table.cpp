#include "world/region_table.h"

namespace Core {

std::string_view RegionTable::getRegionName(RegionId regionId) {
    switch (regionId) {
    case RegionId::None: return "None";
    case RegionId::Ocean: return "Ocean";
    case RegionId::Coast: return "Coastland";
    case RegionId::Plains: return "Plains";
    case RegionId::Forest: return "Woodlands";
    case RegionId::Highlands: return "Highlands";
    case RegionId::Mountains: return "Mountains";
    case RegionId::Settlement: return "Settlement";
    default: return "Unknown";
    }
}

std::string_view RegionTable::getRegionShortName(RegionId regionId) {
    switch (regionId) {
    case RegionId::None: return "---";
    case RegionId::Ocean: return "OCN";
    case RegionId::Coast: return "CST";
    case RegionId::Plains: return "PLN";
    case RegionId::Forest: return "WLD";
    case RegionId::Highlands: return "HLD";
    case RegionId::Mountains: return "MTN";
    case RegionId::Settlement: return "STL";
    default: return "???";
    }
}

int32_t RegionTable::getPlayableRegionCount() {
    return static_cast<int32_t>(RegionId::COUNT) - 1;
}

} // namespace Core
