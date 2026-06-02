#include "world/landmark_table.h"
#include <catch2/catch_test_macros.hpp>
#include <cstring>

using Core::findLandmarkIndexAtTile;
using Core::getLandmarkCount;
using Core::getLandmarkDefinition;
using Core::getLandmarkTooltipText;
using Core::LandmarkDefinition;

TEST_CASE("LandmarkTable contains expected districts", "[landmark_table]") {
    const int32_t expectedLandmarkCount = 33;
    REQUIRE(getLandmarkCount() == expectedLandmarkCount);
    const LandmarkDefinition* timesSquare = getLandmarkDefinition(findLandmarkIndexAtTile(234, 192));
    REQUIRE(timesSquare != nullptr);
    REQUIRE(std::strcmp(timesSquare->fullName, "Times Square") == 0);
    const int32_t lgaIndex = findLandmarkIndexAtTile(331, 170);
    REQUIRE(lgaIndex >= 0);
    const LandmarkDefinition* lga = getLandmarkDefinition(lgaIndex);
    REQUIRE(lga != nullptr);
    REQUIRE(std::strcmp(lga->mapLabel, "LGA") == 0);
    REQUIRE(std::strcmp(getLandmarkTooltipText(lgaIndex), "LaGuardia Airport") == 0);
}
