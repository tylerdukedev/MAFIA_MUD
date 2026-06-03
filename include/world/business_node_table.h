#pragma once

#include "core/types.h"
#include <cstdint>

namespace Core {

constexpr int32_t MAX_BUSINESS_NODE_COUNT = 48;

struct BusinessNodeDefinition {
    const char* id;
    int32_t tileX;
    int32_t tileY;
    const char* fullName;
    const char* mapLabel;
    int64_t jobWageCents;
    float minNetworkAccess;
};

int32_t getBusinessNodeCount();
const BusinessNodeDefinition* getBusinessNodeDefinition(int32_t businessIndex);
int32_t findBusinessNodeIndexAtTile(int32_t tileX, int32_t tileY);
RegionId getBusinessNodeRegionId(int32_t businessIndex);

} // namespace Core
