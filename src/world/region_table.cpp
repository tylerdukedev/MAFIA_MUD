#include "world/region_table.h"

namespace Core {

std::string_view RegionTable::getRegionName(RegionId regionId) {
    switch (regionId) {
    case RegionId::None: return "Water";
    case RegionId::Manhattan: return "Manhattan";
    case RegionId::Brooklyn: return "Brooklyn";
    case RegionId::Queens: return "Queens";
    case RegionId::Bronx: return "The Bronx";
    case RegionId::StatenIsland: return "Staten Island";
    case RegionId::NewJersey: return "New Jersey";
    case RegionId::LongIsland: return "Long Island";
    default: return "Unknown";
    }
}

std::string_view RegionTable::getRegionShortName(RegionId regionId) {
    switch (regionId) {
    case RegionId::None: return "---";
    case RegionId::Manhattan: return "MAN";
    case RegionId::Brooklyn: return "BKL";
    case RegionId::Queens: return "QNS";
    case RegionId::Bronx: return "BRX";
    case RegionId::StatenIsland: return "SI";
    case RegionId::NewJersey: return "NJ";
    case RegionId::LongIsland: return "LI";
    default: return "???";
    }
}

int32_t RegionTable::getPlayableRegionCount() {
    return static_cast<int32_t>(RegionId::COUNT) - 1;
}

} // namespace Core
