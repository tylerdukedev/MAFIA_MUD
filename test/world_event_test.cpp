#include "game/housing_economy.h"
#include "game/housing_living_costs.h"
#include "game/player_operations.h"
#include "sim/world_event_catalog.h"
#include "sim/world_event_system.h"
#include "world/tile_vitality.h"
#include <catch2/catch_test_macros.hpp>

using namespace Core;

TEST_CASE("Housing rent multiplier tracks borough economic health", "[world_event]") {
    BoroughVitalityStore vitality{};
    vitality.snapshots[static_cast<size_t>(RegionId::Manhattan)].economicHealth = 80.0f;
    vitality.snapshots[static_cast<size_t>(RegionId::Manhattan)].playerInfluenceSum = 500.0f;
    ChunkStore chunkStore(WorldConfig{});
    const int32_t strongEconomyBps = computeHousingRentMultiplierBps(vitality, chunkStore, static_cast<uint8_t>(RegionId::Manhattan));
    vitality.snapshots[static_cast<size_t>(RegionId::Manhattan)].economicHealth = 20.0f;
    const int32_t weakEconomyBps = computeHousingRentMultiplierBps(vitality, chunkStore, static_cast<uint8_t>(RegionId::Manhattan));
    REQUIRE(strongEconomyBps > weakEconomyBps);
}

TEST_CASE("Missed rent months trigger eviction event condition", "[world_event]") {
    PlayerOperationsStore operations{};
    operations.headquartersKind = HeadquartersKind::RentedRoom;
    operations.consecutiveUnpaidRentMonths = MISSED_RENT_MONTHS_BEFORE_EVICTION;
    WorldEventStore eventStore{};
    PlayerWallet wallet{};
    CharacterAgentStore agents{};
    BoroughVitalityStore vitality{};
    const WorldEventDefinition* definition = getWorldEventDefinition(findWorldEventDefinitionIndexById("eviction_missed_rent"));
    REQUIRE(definition != nullptr);
    REQUIRE(evaluateWorldEventCondition(definition->condition, eventStore, operations, wallet, agents, vitality));
}
