#include "world/landmark_table.h"
#include <catch2/catch_test_macros.hpp>
#include <cstring>

using Core::findLandmarkIndexAtTile;
using Core::getLandmarkCount;
using Core::getLandmarkDefinition;
using Core::getLandmarkTooltipText;
using Core::LandmarkDefinition;

TEST_CASE("LandmarkTable contains expected districts", "[landmark_table]") {
    const int32_t expectedLandmarkCount = 39;
    REQUIRE(getLandmarkCount() == expectedLandmarkCount);
    REQUIRE(getLandmarkCount() <= Core::MAX_LANDMARK_COUNT);
    const LandmarkDefinition* timesSquare = getLandmarkDefinition(findLandmarkIndexAtTile(234, 192));
    REQUIRE(timesSquare != nullptr);
    REQUIRE(std::strcmp(timesSquare->fullName, "Times Square") == 0);
    const int32_t lgaIndex = findLandmarkIndexAtTile(331, 170);
    REQUIRE(lgaIndex >= 0);
    const LandmarkDefinition* lga = getLandmarkDefinition(lgaIndex);
    REQUIRE(lga != nullptr);
    REQUIRE(std::strcmp(lga->mapLabel, "LGA") == 0);
    REQUIRE(std::strcmp(getLandmarkTooltipText(lgaIndex), "LaGuardia Airport") == 0);
    const int32_t hellsKitchenIndex = findLandmarkIndexAtTile(248, 215);
    REQUIRE(hellsKitchenIndex >= 0);
    const LandmarkDefinition* hellsKitchen = getLandmarkDefinition(hellsKitchenIndex);
    REQUIRE(hellsKitchen != nullptr);
    REQUIRE(hellsKitchen->economicWeightBonus >= Core::LANDMARK_DEFAULT_ECONOMIC_WEIGHT_BONUS);
}
