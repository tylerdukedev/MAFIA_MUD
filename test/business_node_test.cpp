#include "world/business_node_table.h"
#include <catch2/catch_test_macros.hpp>

using namespace Core;

TEST_CASE("BusinessNodeTable includes Newark freight near moved Penn Station", "[business_node]") {
    const int32_t newarkFreightIndex = findBusinessNodeIndexAtTile(115, 176);
    REQUIRE(newarkFreightIndex >= 0);
    const BusinessNodeDefinition* business = getBusinessNodeDefinition(newarkFreightIndex);
    REQUIRE(business != nullptr);
    REQUIRE(getBusinessNodeRegionId(newarkFreightIndex) == RegionId::NewJersey);
}

TEST_CASE("BusinessNodeTable region ids align with every business index", "[business_node]") {
    const int32_t businessCount = getBusinessNodeCount();
    REQUIRE(businessCount >= 20);
    for (int32_t businessIndex = 0; businessIndex < businessCount; ++businessIndex) {
        const BusinessNodeDefinition* business = getBusinessNodeDefinition(businessIndex);
        REQUIRE(business != nullptr);
        const RegionId regionId = getBusinessNodeRegionId(businessIndex);
        REQUIRE(regionId != RegionId::None);
        if (isLawOfficeBusinessIndex(businessIndex)) {
            REQUIRE(business->jobWageCents == 0);
        } else {
            REQUIRE(business->jobWageCents > 0);
            REQUIRE(business->wageMultiplier >= 0.85f);
            REQUIRE(business->wageMultiplier <= 1.25f);
        }
    }
}
