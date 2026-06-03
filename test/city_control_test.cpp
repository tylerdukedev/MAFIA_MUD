#include "world/city_control.h"
#include <catch2/catch_test_macros.hpp>

using namespace Core;

TEST_CASE("CityControlStore claim and ownership", "[world]") {
    CityControlStore store{};
    resetCityControlStore(store);
    REQUIRE_FALSE(isCityClaimed(store, 0));
    REQUIRE(tryClaimCityForPlayer(store, 0, 10U));
    REQUIRE(isCityClaimed(store, 0));
    REQUIRE(getCityOwnerId(store, 0) == PLAYER_OWNER_ID);
    REQUIRE(countPlayerOwnedCities(store) == 1);
    REQUIRE_FALSE(tryClaimCityForPlayer(store, 0, 11U));
}
