#pragma once

#include "core/types.h"
#include <string_view>

namespace Core {

class RegionTable {
public:
    static std::string_view getRegionName(RegionId regionId);
    static std::string_view getRegionShortName(RegionId regionId);
    static int32_t getPlayableRegionCount();
};

} // namespace Core
