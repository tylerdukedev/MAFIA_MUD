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
