#include "world/tile_vitality.h"
#include "utils/seed_hash.h"
#include "world/landmark_table.h"
#include "world/region_table.h"
#include <algorithm>
#include <cmath>

namespace Core {

namespace {
constexpr float MANHATTAN_WEIGHT_SCALE = 1.35f;
constexpr float BOROUGH_POPULATION_BASE = 850000.0f;
constexpr float BIRTH_RATE_PER_TICK = 0.000008f;
constexpr float DEATH_RATE_PER_TICK = 0.000006f;
constexpr float MIGRATION_HEALTH_SCALE = 0.00004f;
constexpr float MIGRATION_CRIME_SCALE = 0.00003f;
constexpr float POPULATION_REDISTRIBUTE_THRESHOLD = 32.0f;
constexpr uint16_t POPULATION_PER_WEIGHT_UNIT = 120;

bool isPlayableRegion(RegionId regionId) {
    return regionId != RegionId::None;
}

int32_t regionToSlot(RegionId regionId) {
    return static_cast<int32_t>(regionId);
}

float boroughPopulationScale(RegionId regionId) {
    switch (regionId) {
    case RegionId::Manhattan: return 1.45f;
    case RegionId::Brooklyn: return 1.25f;
    case RegionId::Queens: return 1.15f;
    case RegionId::Bronx: return 0.95f;
    case RegionId::StatenIsland: return 0.55f;
    case RegionId::NewJersey: return 0.70f;
    case RegionId::LongIsland: return 0.65f;
    default: return 0.0f;
    }
}
} // namespace

uint8_t computeBaseEconomicWeight(RegionId regionId, TerrainId terrainId) {
    if (terrainId == TerrainId::Water || regionId == RegionId::None) {
        return 0;
    }
    uint8_t weight = 40;
    if (terrainId == TerrainId::Building) {
        weight = 72;
    } else if (terrainId == TerrainId::Road) {
        weight = 58;
    } else if (terrainId == TerrainId::Plaza) {
        weight = 88;
    } else if (terrainId == TerrainId::OpenLand) {
        weight = 64;
    } else if (terrainId == TerrainId::Park) {
        weight = 36;
    }
    if (regionId == RegionId::Manhattan) {
        const float scaled = static_cast<float>(weight) * MANHATTAN_WEIGHT_SCALE;
        weight = static_cast<uint8_t>(std::min(255.0f, scaled));
    }
    return weight;
}

uint8_t scalePressureToUint8(float normalizedValue) {
    const float clamped = std::max(0.0f, std::min(1.0f, normalizedValue));
    return static_cast<uint8_t>(clamped * static_cast<float>(TILE_PRESSURE_MAX));
}

float pressureUint8ToNormalized(uint8_t pressure) {
    return static_cast<float>(pressure) / static_cast<float>(TILE_PRESSURE_MAX);
}

float computeBoroughEconomicHealth(const BoroughRollupAccumulator& accumulator) {
    if (accumulator.weightSum <= 0.0f) {
        return 50.0f;
    }
    const float legitActivity = accumulator.weightedBusinessSum / accumulator.weightSum;
    const float crimePenalty = accumulator.weightedCrimeSum / accumulator.weightSum;
    const float lawStability = accumulator.weightedLawSum / accumulator.weightSum;
    const float playerBoost = accumulator.weightedPlayerSum / accumulator.weightSum;
    const float oppositionDrag = accumulator.weightedOppositionSum / accumulator.weightSum;
    const float health = legitActivity * 55.0f
        - crimePenalty * 28.0f
        + lawStability * 12.0f
        + playerBoost * 18.0f
        - oppositionDrag * 14.0f
        + 25.0f;
    return std::max(BOROUGH_HEALTH_MIN, std::min(BOROUGH_HEALTH_MAX, health));
}

float computeBoroughCrimeRate(const BoroughRollupAccumulator& accumulator) {
    if (accumulator.weightSum <= 0.0f) {
        return 0.0f;
    }
    const float crime = accumulator.weightedCrimeSum / accumulator.weightSum;
    const float law = accumulator.weightedLawSum / accumulator.weightSum;
    const float rate = crime / (1.0f + law * 0.65f);
    return std::max(0.0f, std::min(1.0f, rate));
}

float computeBoroughUnemploymentRate(const BoroughRollupAccumulator& accumulator) {
    if (accumulator.weightSum <= 0.0f) {
        return 0.0f;
    }
    const float business = accumulator.weightedBusinessSum / accumulator.weightSum;
    return std::max(0.0f, std::min(1.0f, 1.0f - business));
}

float computeBoroughTaxYieldIndex(const BoroughRollupAccumulator& accumulator) {
    if (accumulator.weightSum <= 0.0f) {
        return 0.0f;
    }
    const float business = accumulator.weightedBusinessSum / accumulator.weightSum;
    const float populationScale = static_cast<float>(accumulator.totalPopulation) / 250000.0f;
    return std::max(0.0f, std::min(100.0f, business * 40.0f + populationScale * 8.0f));
}

void initializeTileVitality(ChunkStore& chunkStore) {
    const WorldConfig& worldConfig = chunkStore.getConfig();
    for (int32_t tileY = 0; tileY < worldConfig.WORLD_HEIGHT_TILES; ++tileY) {
        for (int32_t tileX = 0; tileX < worldConfig.WORLD_WIDTH_TILES; ++tileX) {
            const WorldCoord coord{tileX, tileY};
            const RegionId regionId = chunkStore.getRegionAt(coord);
            const TerrainId terrainId = chunkStore.getTerrainAt(coord);
            const uint8_t weight = computeBaseEconomicWeight(regionId, terrainId);
            chunkStore.setEconomicWeightAt(coord, weight);
            const uint16_t population = static_cast<uint16_t>(
                std::min(65535, static_cast<int32_t>(weight) * static_cast<int32_t>(POPULATION_PER_WEIGHT_UNIT)));
            chunkStore.setPopulationAt(coord, population);
            chunkStore.setCrimePressureAt(coord, TILE_PRESSURE_NEUTRAL);
            chunkStore.setLawPressureAt(coord, TILE_PRESSURE_NEUTRAL);
            chunkStore.setBusinessVitalityAt(coord, TILE_PRESSURE_NEUTRAL);
            chunkStore.setPlayerInfluenceAt(coord, 0);
            chunkStore.setOppositionInfluenceAt(coord, TILE_PRESSURE_NEUTRAL / 4);
        }
    }
    applyLandmarkVitalitySeeds(chunkStore);
}

void applyLandmarkVitalitySeeds(ChunkStore& chunkStore) {
    const WorldConfig& worldConfig = chunkStore.getConfig();
    const int32_t landmarkCount = getLandmarkCount();
    for (int32_t landmarkIndex = 0; landmarkIndex < landmarkCount; ++landmarkIndex) {
        const LandmarkDefinition* landmark = getLandmarkDefinition(landmarkIndex);
        if (landmark == nullptr) {
            continue;
        }
        const int32_t radius = static_cast<int32_t>(landmark->heatRadiusTiles);
        for (int32_t offsetY = -radius; offsetY <= radius; ++offsetY) {
            for (int32_t offsetX = -radius; offsetX <= radius; ++offsetX) {
                const WorldCoord coord{landmark->tileX + offsetX, landmark->tileY + offsetY};
                if (!worldConfig.isWithinWorldBounds(coord)) {
                    continue;
                }
                if (chunkStore.getTerrainAt(coord) == TerrainId::Water) {
                    continue;
                }
                const uint8_t boostedWeight = static_cast<uint8_t>(std::min(
                    255,
                    static_cast<int32_t>(landmark->economicWeightBonus)));
                const uint8_t currentWeight = chunkStore.getEconomicWeightAt(coord);
                chunkStore.setEconomicWeightAt(coord, std::max(currentWeight, boostedWeight));
                const uint8_t heatCrime = scalePressureToUint8(LANDMARK_BASE_HEAT);
                chunkStore.setCrimePressureAt(coord, std::max(chunkStore.getCrimePressureAt(coord), heatCrime));
                const uint8_t businessBoost = scalePressureToUint8(0.72f);
                chunkStore.setBusinessVitalityAt(coord, std::max(chunkStore.getBusinessVitalityAt(coord), businessBoost));
            }
        }
    }
}

void rollupBoroughVitality(const WorldConfig& worldConfig, ChunkStore& chunkStore, BoroughVitalityStore& store) {
    BoroughRollupAccumulator accumulators[BOROUGH_VITALITY_SLOT_COUNT] = {};
    for (int32_t chunkIndex = 0; chunkIndex < chunkStore.getTotalChunkCount(); ++chunkIndex) {
        const Chunk* chunk = chunkStore.getChunk(static_cast<ChunkId>(chunkIndex));
        if (chunk == nullptr) {
            continue;
        }
        const ChunkCoord chunkCoord = chunk->coord;
        for (int32_t localY = 0; localY < worldConfig.CHUNK_SIZE; ++localY) {
            for (int32_t localX = 0; localX < worldConfig.CHUNK_SIZE; ++localX) {
                const LocalTileCoord localCoord{
                    static_cast<uint16_t>(localX),
                    static_cast<uint16_t>(localY)};
                const WorldCoord worldCoord = worldConfig.chunkAndLocalToWorld(chunkCoord, localCoord);
                if (!worldConfig.isWithinWorldBounds(worldCoord)) {
                    continue;
                }
                const RegionId regionId = chunkStore.getRegionAt(worldCoord);
                if (!isPlayableRegion(regionId)) {
                    continue;
                }
                const int32_t slot = regionToSlot(regionId);
                BoroughRollupAccumulator& accumulator = accumulators[slot];
                const uint8_t weight = chunkStore.getEconomicWeightAt(worldCoord);
                if (weight == 0) {
                    continue;
                }
                const float weightFloat = static_cast<float>(weight);
                const float landmarkScale = findLandmarkIndexAtTile(worldCoord.x, worldCoord.y) >= 0
                    ? LANDMARK_BOROUGH_INFLUENCE_WEIGHT
                    : 1.0f;
                const float effectiveWeight = weightFloat * landmarkScale;
                accumulator.weightSum += effectiveWeight;
                accumulator.weightedEconomicSum += effectiveWeight * weightFloat;
                accumulator.weightedBusinessSum += effectiveWeight * pressureUint8ToNormalized(
                    chunkStore.getBusinessVitalityAt(worldCoord));
                accumulator.weightedCrimeSum += effectiveWeight * pressureUint8ToNormalized(
                    chunkStore.getCrimePressureAt(worldCoord));
                accumulator.weightedLawSum += effectiveWeight * pressureUint8ToNormalized(
                    chunkStore.getLawPressureAt(worldCoord));
                accumulator.weightedPlayerSum += effectiveWeight * pressureUint8ToNormalized(
                    chunkStore.getPlayerInfluenceAt(worldCoord));
                accumulator.weightedOppositionSum += effectiveWeight * pressureUint8ToNormalized(
                    chunkStore.getOppositionInfluenceAt(worldCoord));
                accumulator.totalPopulation += chunkStore.getPopulationAt(worldCoord);
            }
        }
    }
    for (int32_t slot = 0; slot < BOROUGH_VITALITY_SLOT_COUNT; ++slot) {
        const RegionId regionId = static_cast<RegionId>(slot);
        if (!isPlayableRegion(regionId)) {
            continue;
        }
        BoroughVitalitySnapshot& snapshot = store.snapshots[slot];
        const BoroughRollupAccumulator& accumulator = accumulators[slot];
        snapshot.totalPopulation = accumulator.totalPopulation;
        snapshot.weightedEconomicSum = accumulator.weightedEconomicSum;
        snapshot.weightedBusinessSum = accumulator.weightedBusinessSum;
        snapshot.economicHealth = computeBoroughEconomicHealth(accumulator);
        snapshot.crimeRate = computeBoroughCrimeRate(accumulator);
        snapshot.unemploymentRate = computeBoroughUnemploymentRate(accumulator);
        snapshot.taxYieldIndex = computeBoroughTaxYieldIndex(accumulator);
        snapshot.playerInfluenceSum = accumulator.weightedPlayerSum;
        snapshot.oppositionInfluenceSum = accumulator.weightedOppositionSum;
        if (store.boroughPopulationTargets[slot] == 0) {
            store.boroughPopulationTargets[slot] = accumulator.totalPopulation;
        }
    }
}

void tickBoroughPopulation(
    BoroughVitalityStore& store,
    RegionId regionId,
    uint64_t tickCount,
    uint64_t worldSeed) {
    if (!isPlayableRegion(regionId)) {
        return;
    }
    const int32_t slot = regionToSlot(regionId);
    BoroughVitalitySnapshot& snapshot = store.snapshots[slot];
    uint32_t population = store.boroughPopulationTargets[slot];
    if (population == 0) {
        population = snapshot.totalPopulation;
    }
    if (population == 0) {
        population = static_cast<uint32_t>(BOROUGH_POPULATION_BASE * boroughPopulationScale(regionId));
    }
    const float populationFloat = static_cast<float>(population);
    const float births = populationFloat * BIRTH_RATE_PER_TICK;
    const float deaths = populationFloat * DEATH_RATE_PER_TICK;
    const float healthDelta = snapshot.economicHealth - 50.0f;
    const float migration = healthDelta * MIGRATION_HEALTH_SCALE * populationFloat
        - snapshot.crimeRate * MIGRATION_CRIME_SCALE * populationFloat;
    const uint32_t hash = Utils::hashSeedMix(worldSeed, static_cast<int32_t>(tickCount), slot);
    const float noise = static_cast<float>(hash % 1000u) / 1000.0f - 0.5f;
    const float adjustedMigration = migration + noise * 2.0f;
    const float nextPopulation = std::max(0.0f, populationFloat + births - deaths + adjustedMigration);
    store.boroughPopulationTargets[slot] = static_cast<uint32_t>(nextPopulation);
}

void tickOpponentPressure(ChunkStore& chunkStore, uint64_t tickCount, uint64_t worldSeed) {
    const WorldConfig& worldConfig = chunkStore.getConfig();
    const uint32_t boroughPick = Utils::hashSeedMix(worldSeed, static_cast<int32_t>(tickCount), 0x0B90) % 7u + 1u;
    const RegionId regionId = static_cast<RegionId>(boroughPick);
    const int32_t sampleCount = 48;
    for (int32_t sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex) {
        const uint32_t hash = Utils::hashSeedMix(worldSeed, static_cast<int32_t>(tickCount), sampleIndex * 17 + 0x1AB);
        const int32_t tileX = static_cast<int32_t>(hash % static_cast<uint32_t>(worldConfig.WORLD_WIDTH_TILES));
        const int32_t tileY = static_cast<int32_t>((hash >> 16) % static_cast<uint32_t>(worldConfig.WORLD_HEIGHT_TILES));
        const WorldCoord coord{tileX, tileY};
        if (chunkStore.getRegionAt(coord) != regionId) {
            continue;
        }
        if (chunkStore.getTerrainAt(coord) == TerrainId::Water) {
            continue;
        }
        const uint8_t law = static_cast<uint8_t>(std::min(
            255,
            static_cast<int32_t>(chunkStore.getLawPressureAt(coord)) + 4));
        chunkStore.setLawPressureAt(coord, law);
        const int32_t player = static_cast<int32_t>(chunkStore.getPlayerInfluenceAt(coord)) - 3;
        chunkStore.setPlayerInfluenceAt(coord, static_cast<uint8_t>(std::max(0, player)));
        const uint8_t opposition = static_cast<uint8_t>(std::min(
            255,
            static_cast<int32_t>(chunkStore.getOppositionInfluenceAt(coord)) + 2));
        chunkStore.setOppositionInfluenceAt(coord, opposition);
    }
}

void distributeBoroughPopulationToTiles(
    const WorldConfig& worldConfig,
    ChunkStore& chunkStore,
    BoroughVitalityStore& store,
    RegionId regionId) {
    if (!isPlayableRegion(regionId)) {
        return;
    }
    const int32_t slot = regionToSlot(regionId);
    const uint32_t targetPopulation = store.boroughPopulationTargets[slot];
    const int32_t currentTotal = static_cast<int32_t>(store.snapshots[slot].totalPopulation);
    const int32_t delta = static_cast<int32_t>(targetPopulation) - currentTotal;
    if (std::abs(delta) < static_cast<int32_t>(POPULATION_REDISTRIBUTE_THRESHOLD)) {
        return;
    }
    uint32_t weightSum = 0;
    for (int32_t tileY = 0; tileY < worldConfig.WORLD_HEIGHT_TILES; ++tileY) {
        for (int32_t tileX = 0; tileX < worldConfig.WORLD_WIDTH_TILES; ++tileX) {
            const WorldCoord coord{tileX, tileY};
            if (chunkStore.getRegionAt(coord) != regionId) {
                continue;
            }
            weightSum += chunkStore.getEconomicWeightAt(coord);
        }
    }
    if (weightSum == 0) {
        return;
    }
    for (int32_t tileY = 0; tileY < worldConfig.WORLD_HEIGHT_TILES; ++tileY) {
        for (int32_t tileX = 0; tileX < worldConfig.WORLD_WIDTH_TILES; ++tileX) {
            const WorldCoord coord{tileX, tileY};
            if (chunkStore.getRegionAt(coord) != regionId) {
                continue;
            }
            const uint8_t weight = chunkStore.getEconomicWeightAt(coord);
            if (weight == 0) {
                chunkStore.setPopulationAt(coord, 0);
                continue;
            }
            const uint32_t tilePopulation = (targetPopulation * weight) / weightSum;
            chunkStore.setPopulationAt(coord, static_cast<uint16_t>(std::min(65535u, tilePopulation)));
        }
    }
}

const BoroughVitalitySnapshot* getBoroughSnapshot(const BoroughVitalityStore& store, RegionId regionId) {
    if (!isPlayableRegion(regionId)) {
        return nullptr;
    }
    return &store.snapshots[regionToSlot(regionId)];
}

void resetBoroughVitalityStore(BoroughVitalityStore& store) {
    for (int32_t slot = 0; slot < BOROUGH_VITALITY_SLOT_COUNT; ++slot) {
        store.snapshots[slot] = BoroughVitalitySnapshot{};
        store.boroughPopulationTargets[slot] = 0;
    }
    store.lastRollupTickCount = 0;
}

} // namespace Core
