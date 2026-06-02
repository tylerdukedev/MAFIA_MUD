#include "world/district_grid.h"
#include "world/world_config.h"
#include <catch2/catch_test_macros.hpp>

using namespace Core;

TEST_CASE("DistrictGrid coordinate conversion", "[world][district]") {
    SECTION("World tile maps to district cell") {
        const DistrictCoord actualCoord = DistrictGrid::worldToDistrictCoord(WorldCoord{0, 0});
        REQUIRE(actualCoord.x == 0);
        REQUIRE(actualCoord.y == 0);
        const DistrictCoord midCoord = DistrictGrid::worldToDistrictCoord(WorldCoord{232, 229});
        REQUIRE(midCoord.x == 14);
        REQUIRE(midCoord.y == 14);
        const DistrictCoord maxCoord = DistrictGrid::worldToDistrictCoord(
            WorldCoord{WorldConfig::WORLD_WIDTH_TILES - 1, WorldConfig::WORLD_HEIGHT_TILES - 1});
        REQUIRE(maxCoord.x == DISTRICT_COUNT - 1);
        REQUIRE(maxCoord.y == DISTRICT_COUNT - 1);
    }
    SECTION("District id round-trips with coord") {
        const DistrictCoord inputCoord{5, 7};
        const DistrictId districtId = DistrictGrid::districtCoordToId(inputCoord);
        const DistrictCoord outputCoord = DistrictGrid::districtIdToCoord(districtId);
        REQUIRE(districtId == 7 * DISTRICT_COUNT + 5);
        REQUIRE(outputCoord.x == inputCoord.x);
        REQUIRE(outputCoord.y == inputCoord.y);
    }
    SECTION("District origin maps back to world origin") {
        const DistrictCoord inputCoord{3, 4};
        const WorldCoord worldOrigin = DistrictGrid::districtToWorldOrigin(inputCoord);
        REQUIRE(worldOrigin.x == 48);
        REQUIRE(worldOrigin.y == 64);
    }
}
