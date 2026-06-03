#include "game/player_operations.h"
#include "character/player_profile.h"
#include "character/profile_builder.h"
#include "character/character_draft.h"
#include <catch2/catch_test_macros.hpp>

using namespace Core;

TEST_CASE("PlayerOperations requires headquarters before city claim", "[player_operations]") {
    PlayerOperationsStore store{};
    PlayerWallet wallet{};
    wallet.cashCents = 500000;
    PlayerProfile profile = buildPlayerProfile(CharacterDraft{});
    profile.draft.backgroundId = BackgroundId::Bookkeeper;
    profile.networkAccess.businessAssociation = 0.5f;
    profile.networkAccess.ethnicNetwork = 0.5f;
    profile.legitimacy.publicFacingJobAccess = 0.5f;
    profile.legitimacy.shellCompanyEase = 0.5f;
    profile.loyaltyBias.kinAlliancePreference = 0.6f;
    profile.loyaltyBias.ethnicFactionResistance = 0.6f;
    int64_t claimCost = 0;
    REQUIRE_FALSE(canEstablishCityOperation(store, profile, wallet, claimCost));
    store.headquartersKind = HeadquartersKind::RentedRoom;
    REQUIRE(canEstablishCityOperation(store, profile, wallet, claimCost));
    REQUIRE(claimCost == CITY_ESTABLISH_COST_CENTS);
}

TEST_CASE("PlayerOperations establishes rented room headquarters", "[player_operations]") {
    PlayerOperationsStore store{};
    PlayerWallet wallet{};
    wallet.cashCents = 20000;
    PlayerProfile profile{};
    REQUIRE(tryEstablishOperation(store, wallet, profile, 0));
    REQUIRE(store.headquartersKind == HeadquartersKind::RentedRoom);
}
