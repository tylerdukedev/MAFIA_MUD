#include "world/region_table.h"

namespace Core {

std::string_view RegionTable::getRegionName(RegionId regionId) {
    switch (regionId) {
    case RegionId::None: return "None";
    case RegionId::Downtown: return "Downtown";
    case RegionId::Midtown: return "Midtown";
    case RegionId::Residential: return "Residential";
    case RegionId::Commercial: return "Commercial";
    case RegionId::Industrial: return "Industrial";
    case RegionId::Waterfront: return "Waterfront";
    case RegionId::Outskirts: return "Outskirts";
    default: return "Unknown";
    }
}

std::string_view RegionTable::getRegionShortName(RegionId regionId) {
    switch (regionId) {
    case RegionId::None: return "---";
    case RegionId::Downtown: return "DTN";
    case RegionId::Midtown: return "MID";
    case RegionId::Residential: return "RES";
    case RegionId::Commercial: return "COM";
    case RegionId::Industrial: return "IND";
    case RegionId::Waterfront: return "WTF";
    case RegionId::Outskirts: return "OUT";
    default: return "???";
    }
}

int32_t RegionTable::getPlayableRegionCount() {
    return static_cast<int32_t>(RegionId::COUNT) - 1;
}

} // namespace Core
