#include "world/tile_vitality.h"
#include "world/world_config.h"
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <cstdlib>

using namespace Core;

namespace {

void seedMockTile(
    ChunkStore& chunkStore,
    const WorldCoord& coord,
    RegionId regionId,
    uint8_t economicWeight,
    uint16_t population,
    uint8_t businessVitality,
    uint8_t crimePressure) {
    chunkStore.setRegionAt(coord, regionId);
    chunkStore.setTerrainAt(coord, TerrainId::Building);
    chunkStore.setEconomicWeightAt(coord, economicWeight);
    chunkStore.setPopulationAt(coord, population);
    chunkStore.setBusinessVitalityAt(coord, businessVitality);
    chunkStore.setCrimePressureAt(coord, crimePressure);
    chunkStore.setLawPressureAt(coord, TILE_PRESSURE_NEUTRAL);
    chunkStore.setPlayerInfluenceAt(coord, 0);
    chunkStore.setOppositionInfluenceAt(coord, TILE_PRESSURE_NEUTRAL / 4);
}

} // namespace

TEST_CASE("Borough economic health stays within bounds", "[tile_vitality]") {
    BoroughRollupAccumulator inputAccumulator{};
    inputAccumulator.weightSum = 100.0f;
    inputAccumulator.weightedBusinessSum = 90.0f;
    inputAccumulator.weightedCrimeSum = 10.0f;
    inputAccumulator.weightedLawSum = 50.0f;
    inputAccumulator.weightedPlayerSum = 20.0f;
    inputAccumulator.weightedOppositionSum = 15.0f;
    const float actualHealth = computeBoroughEconomicHealth(inputAccumulator);
    REQUIRE(actualHealth >= BOROUGH_HEALTH_MIN);
    REQUIRE(actualHealth <= BOROUGH_HEALTH_MAX);
}

TEST_CASE("Rollup and population redistribution conserve totals", "[tile_vitality]") {
    WorldConfig inputConfig;
    ChunkStore inputStore(inputConfig);
    BoroughVitalityStore inputStoreVitality{};
    const RegionId inputRegion = RegionId::Manhattan;
    seedMockTile(inputStore, WorldCoord{10, 10}, inputRegion, 80, 800, 200, 140);
    seedMockTile(inputStore, WorldCoord{11, 10}, inputRegion, 40, 400, 180, 120);
    seedMockTile(inputStore, WorldCoord{10, 11}, inputRegion, 20, 200, 160, 100);
    rollupBoroughVitality(inputConfig, inputStore, inputStoreVitality);
    const BoroughVitalitySnapshot* actualSnapshot = getBoroughSnapshot(inputStoreVitality, inputRegion);
    REQUIRE(actualSnapshot != nullptr);
    REQUIRE(actualSnapshot->totalPopulation == 1400u);
    inputStoreVitality.boroughPopulationTargets[static_cast<int32_t>(inputRegion)] = 2000u;
    distributeBoroughPopulationToTiles(inputConfig, inputStore, inputStoreVitality, inputRegion);
    uint32_t actualRedistributedTotal = 0;
    actualRedistributedTotal += inputStore.getPopulationAt(WorldCoord{10, 10});
    actualRedistributedTotal += inputStore.getPopulationAt(WorldCoord{11, 10});
    actualRedistributedTotal += inputStore.getPopulationAt(WorldCoord{10, 11});
    const int32_t expectedTotal = 2000;
    const int32_t actualTotal = static_cast<int32_t>(actualRedistributedTotal);
    REQUIRE(std::abs(actualTotal - expectedTotal) <= 3);
}
