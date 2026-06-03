#pragma once

#include "core/types.h"
#include "world/chunk_store.h"
#include "world/world_config.h"
#include <cstdint>

namespace Core {

constexpr int32_t BOROUGH_VITALITY_SLOT_COUNT = static_cast<int32_t>(RegionId::COUNT);
constexpr int32_t BOROUGH_ROLLUP_INTERVAL_TICKS = 40;
constexpr int32_t OPPONENT_LAW_PRESSURE_INTERVAL_TICKS = 200;
constexpr uint8_t TILE_PRESSURE_NEUTRAL = 128;
constexpr uint8_t TILE_PRESSURE_MAX = 255;
constexpr float BOROUGH_HEALTH_MIN = 0.0f;
constexpr float BOROUGH_HEALTH_MAX = 100.0f;

struct BoroughVitalitySnapshot {
    float economicHealth = 50.0f;
    uint32_t totalPopulation = 0;
    float crimeRate = 0.0f;
    float unemploymentRate = 0.0f;
    float taxYieldIndex = 0.0f;
    float playerInfluenceSum = 0.0f;
    float oppositionInfluenceSum = 0.0f;
    float weightedEconomicSum = 0.0f;
    float weightedBusinessSum = 0.0f;
};

struct BoroughVitalityStore {
    BoroughVitalitySnapshot snapshots[BOROUGH_VITALITY_SLOT_COUNT];
    uint64_t lastRollupTickCount = 0;
    uint32_t boroughPopulationTargets[BOROUGH_VITALITY_SLOT_COUNT] = {};
};

struct BoroughRollupAccumulator {
    float weightedEconomicSum = 0.0f;
    float weightSum = 0.0f;
    float weightedBusinessSum = 0.0f;
    float weightedCrimeSum = 0.0f;
    float weightedLawSum = 0.0f;
    float weightedPlayerSum = 0.0f;
    float weightedOppositionSum = 0.0f;
    uint32_t totalPopulation = 0;
};

uint8_t computeBaseEconomicWeight(RegionId regionId, TerrainId terrainId);
uint8_t scalePressureToUint8(float normalizedValue);
float pressureUint8ToNormalized(uint8_t pressure);
float computeBoroughEconomicHealth(const BoroughRollupAccumulator& accumulator);
float computeBoroughCrimeRate(const BoroughRollupAccumulator& accumulator);
float computeBoroughUnemploymentRate(const BoroughRollupAccumulator& accumulator);
float computeBoroughTaxYieldIndex(const BoroughRollupAccumulator& accumulator);
void initializeTileVitality(ChunkStore& chunkStore);
void applyLandmarkVitalitySeeds(ChunkStore& chunkStore);
void rollupBoroughVitality(const WorldConfig& worldConfig, ChunkStore& chunkStore, BoroughVitalityStore& store);
void tickBoroughPopulation(
    BoroughVitalityStore& store,
    RegionId regionId,
    uint64_t tickCount,
    uint64_t worldSeed);
void tickOpponentPressure(ChunkStore& chunkStore, uint64_t tickCount, uint64_t worldSeed);
void distributeBoroughPopulationToTiles(
    const WorldConfig& worldConfig,
    ChunkStore& chunkStore,
    BoroughVitalityStore& store,
    RegionId regionId);

const BoroughVitalitySnapshot* getBoroughSnapshot(
    const BoroughVitalityStore& store,
    RegionId regionId);

void resetBoroughVitalityStore(BoroughVitalityStore& store);

} // namespace Core
