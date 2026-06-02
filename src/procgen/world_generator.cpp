#include "procgen/world_generator.h"
#include "utils/seed_hash.h"
#include <cmath>
#include <cstdint>

namespace Core {

namespace {
constexpr int32_t AVENUE_SPACING = 8;
constexpr int32_t STREET_SPACING = 4;
constexpr int32_t BOULEVARD_SPACING = 32;
constexpr float DISTANCE_NORMALIZER = 256.0f;
} // namespace

void WorldGenerator::deriveLayoutParams() {
    const uint32_t seedMixA = Utils::hashSeedMix(worldSeed, 11, 29);
    const uint32_t seedMixB = Utils::hashSeedMix(worldSeed, 47, 83);
    const uint32_t seedMixC = Utils::hashSeedMix(worldSeed, 101, 157);
    layoutParams.cityCenterX = 248.0f + static_cast<float>(seedMixA % 24U);
    layoutParams.cityCenterY = 248.0f + static_cast<float>(seedMixB % 24U);
    layoutParams.cityRadius = 220.0f + static_cast<float>(seedMixC % 30U);
    layoutParams.verticalRiverX = 132 + static_cast<int32_t>(seedMixA % 28U);
    layoutParams.eastRiverWestX = 308 + static_cast<int32_t>(seedMixB % 18U);
    layoutParams.eastRiverEastX = layoutParams.eastRiverWestX + 18 + static_cast<int32_t>(seedMixC % 8U);
    layoutParams.harborNorthY = 72 + static_cast<int32_t>(seedMixA % 28U);
    layoutParams.parkCenterX = static_cast<int32_t>(layoutParams.cityCenterX) - 8 + static_cast<int32_t>(seedMixB % 16U);
    layoutParams.parkCenterY = static_cast<int32_t>(layoutParams.cityCenterY) - 72 + static_cast<int32_t>(seedMixC % 16U);
    layoutParams.parkHalfWidth = 30 + static_cast<int32_t>(seedMixA % 10U);
    layoutParams.parkHalfHeight = 20 + static_cast<int32_t>(seedMixB % 8U);
}

void WorldGenerator::generate(const WorldConfig& worldConfig, ChunkStore& chunkStore, uint64_t inputSeed) {
    worldSeed = inputSeed;
    deriveLayoutParams();
    passGeography(worldConfig, chunkStore);
    passUrbanFootprint(worldConfig, chunkStore);
    passStreetGrid(worldConfig, chunkStore);
    passDistricts(worldConfig, chunkStore);
    passCityBlocks(worldConfig, chunkStore);
}

bool WorldGenerator::isWaterTile(int32_t x, int32_t y) const {
    if (x < 8 || x > 503 || y < 8 || y > 503) {
        return true;
    }
    if (y < layoutParams.harborNorthY && x > layoutParams.verticalRiverX + 40 && x < layoutParams.eastRiverEastX + 90) {
        return true;
    }
    if (x < layoutParams.verticalRiverX) {
        return true;
    }
    if (x > layoutParams.eastRiverWestX && x < layoutParams.eastRiverEastX && y > layoutParams.harborNorthY && y < 430) {
        return true;
    }
    const float deltaX = static_cast<float>(x) - layoutParams.cityCenterX;
    const float deltaY = static_cast<float>(y) - (layoutParams.cityCenterY + 120.0f);
    if ((deltaX * deltaX) / 3600.0f + (deltaY * deltaY) / 1600.0f < 1.0f && y > 360) {
        return true;
    }
    return false;
}

bool WorldGenerator::isUrbanTile(int32_t x, int32_t y) const {
    if (isWaterTile(x, y)) {
        return false;
    }
    const float deltaX = static_cast<float>(x) - layoutParams.cityCenterX;
    const float deltaY = static_cast<float>(y) - layoutParams.cityCenterY;
    const float distance = std::sqrt(deltaX * deltaX + deltaY * deltaY);
    const uint32_t edgeHash = Utils::hashSeedMix(worldSeed, x, y);
    const float edgeNoise = static_cast<float>(edgeHash % 1000U) / 1000.0f * 28.0f;
    if (distance <= layoutParams.cityRadius + edgeNoise) {
        return true;
    }
    if (x > layoutParams.verticalRiverX && x < layoutParams.eastRiverWestX && y > layoutParams.harborNorthY && y < 420) {
        return true;
    }
    return false;
}

bool WorldGenerator::isRoadTile(int32_t x, int32_t y) const {
    if (!isUrbanTile(x, y)) {
        return false;
    }
    if (x % BOULEVARD_SPACING == 0 || y % BOULEVARD_SPACING == 0) {
        return true;
    }
    if (x % AVENUE_SPACING == 0 || y % STREET_SPACING == 0) {
        return true;
    }
    if (y == layoutParams.harborNorthY + 2 && x > layoutParams.verticalRiverX && x < layoutParams.eastRiverEastX + 40) {
        return true;
    }
    return false;
}

bool WorldGenerator::isCentralParkTile(int32_t x, int32_t y) const {
    const int32_t deltaX = x - layoutParams.parkCenterX;
    const int32_t deltaY = y - layoutParams.parkCenterY;
    return std::abs(deltaX) <= layoutParams.parkHalfWidth && std::abs(deltaY) <= layoutParams.parkHalfHeight;
}

bool WorldGenerator::isMajorPlazaTile(int32_t x, int32_t y) const {
    if (!isUrbanTile(x, y)) {
        return false;
    }
    const bool isBoulevardCrossing = (x % BOULEVARD_SPACING == 0) && (y % BOULEVARD_SPACING == 0);
    if (!isBoulevardCrossing) {
        return false;
    }
    const uint32_t plazaHash = Utils::hashSeedMix(worldSeed, x / BOULEVARD_SPACING, y / BOULEVARD_SPACING);
    return (plazaHash % 5U) == 0U;
}

RegionId WorldGenerator::pickDistrict(int32_t x, int32_t y) const {
    const float deltaX = static_cast<float>(x) - layoutParams.cityCenterX;
    const float deltaY = static_cast<float>(y) - layoutParams.cityCenterY;
    const float distance = std::sqrt(deltaX * deltaX + deltaY * deltaY) / DISTANCE_NORMALIZER;
    if (isWaterTile(x + 1, y) || isWaterTile(x - 1, y) || isWaterTile(x, y + 1) || isWaterTile(x, y - 1)) {
        return RegionId::Waterfront;
    }
    if (distance < 0.16f) {
        return RegionId::Downtown;
    }
    if (distance < 0.30f) {
        return RegionId::Midtown;
    }
    if (distance < 0.46f) {
        return RegionId::Commercial;
    }
    if (deltaY > 40.0f && distance < 0.62f) {
        return RegionId::Industrial;
    }
    if (distance > 0.72f) {
        return RegionId::Outskirts;
    }
    return RegionId::Residential;
}

void WorldGenerator::passGeography(const WorldConfig& worldConfig, ChunkStore& chunkStore) {
    for (int32_t y = 0; y < worldConfig.WORLD_HEIGHT_TILES; ++y) {
        for (int32_t x = 0; x < worldConfig.WORLD_WIDTH_TILES; ++x) {
            WorldCoord coord{x, y};
            if (isWaterTile(x, y)) {
                chunkStore.setTerrainAt(coord, TerrainId::Water);
                chunkStore.setRegionAt(coord, RegionId::None);
                chunkStore.setElevationAt(coord, 0);
                continue;
            }
            chunkStore.setTerrainAt(coord, TerrainId::OpenLand);
            chunkStore.setRegionAt(coord, RegionId::Outskirts);
            chunkStore.setElevationAt(coord, 2);
        }
    }
}

void WorldGenerator::passUrbanFootprint(const WorldConfig& worldConfig, ChunkStore& chunkStore) {
    for (int32_t y = 0; y < worldConfig.WORLD_HEIGHT_TILES; ++y) {
        for (int32_t x = 0; x < worldConfig.WORLD_WIDTH_TILES; ++x) {
            WorldCoord coord{x, y};
            if (chunkStore.getTerrainAt(coord) == TerrainId::Water) {
                continue;
            }
            if (!isUrbanTile(x, y)) {
                continue;
            }
            chunkStore.setTerrainAt(coord, TerrainId::Building);
            chunkStore.setRegionAt(coord, RegionId::Residential);
        }
    }
}

void WorldGenerator::passStreetGrid(const WorldConfig& worldConfig, ChunkStore& chunkStore) {
    for (int32_t y = 0; y < worldConfig.WORLD_HEIGHT_TILES; ++y) {
        for (int32_t x = 0; x < worldConfig.WORLD_WIDTH_TILES; ++x) {
            WorldCoord coord{x, y};
            if (chunkStore.getTerrainAt(coord) == TerrainId::Water) {
                continue;
            }
            if (!isRoadTile(x, y)) {
                continue;
            }
            chunkStore.setTerrainAt(coord, TerrainId::Road);
        }
    }
}

void WorldGenerator::passDistricts(const WorldConfig& worldConfig, ChunkStore& chunkStore) {
    for (int32_t y = 0; y < worldConfig.WORLD_HEIGHT_TILES; ++y) {
        for (int32_t x = 0; x < worldConfig.WORLD_WIDTH_TILES; ++x) {
            WorldCoord coord{x, y};
            const TerrainId terrainId = chunkStore.getTerrainAt(coord);
            if (terrainId == TerrainId::Water || terrainId == TerrainId::OpenLand) {
                continue;
            }
            chunkStore.setRegionAt(coord, pickDistrict(x, y));
        }
    }
}

void WorldGenerator::passCityBlocks(const WorldConfig& worldConfig, ChunkStore& chunkStore) {
    for (int32_t y = 0; y < worldConfig.WORLD_HEIGHT_TILES; ++y) {
        for (int32_t x = 0; x < worldConfig.WORLD_WIDTH_TILES; ++x) {
            WorldCoord coord{x, y};
            TerrainId terrainId = chunkStore.getTerrainAt(coord);
            if (terrainId == TerrainId::Water) {
                continue;
            }
            if (isCentralParkTile(x, y) && terrainId != TerrainId::Road) {
                chunkStore.setTerrainAt(coord, TerrainId::Park);
                chunkStore.setRegionAt(coord, RegionId::Midtown);
                chunkStore.setElevationAt(coord, 4);
                continue;
            }
            if (terrainId == TerrainId::Road && isMajorPlazaTile(x, y)) {
                chunkStore.setTerrainAt(coord, TerrainId::Plaza);
                chunkStore.setElevationAt(coord, 3);
                continue;
            }
            if (terrainId == TerrainId::Road) {
                chunkStore.setElevationAt(coord, 1);
                continue;
            }
            if (terrainId == TerrainId::OpenLand) {
                const uint32_t hash = Utils::hashSeedMix(worldSeed, x, y);
                chunkStore.setElevationAt(coord, static_cast<int16_t>(2 + (hash % 6U)));
                continue;
            }
            const RegionId districtId = chunkStore.getRegionAt(coord);
            const uint32_t hash = Utils::hashSeedMix(worldSeed, x, y);
            int16_t baseHeight = 8;
            switch (districtId) {
            case RegionId::Downtown: baseHeight = 48; break;
            case RegionId::Midtown: baseHeight = 32; break;
            case RegionId::Commercial: baseHeight = 24; break;
            case RegionId::Industrial: baseHeight = 14; break;
            case RegionId::Waterfront: baseHeight = 10; break;
            case RegionId::Outskirts: baseHeight = 6; break;
            default: baseHeight = 12; break;
            }
            const int16_t variation = static_cast<int16_t>(hash % 12U);
            if ((hash % 17U) == 0U && districtId != RegionId::Downtown) {
                chunkStore.setTerrainAt(coord, TerrainId::Park);
                chunkStore.setElevationAt(coord, 4);
                continue;
            }
            chunkStore.setTerrainAt(coord, TerrainId::Building);
            chunkStore.setElevationAt(coord, static_cast<int16_t>(baseHeight + variation));
        }
    }
}

} // namespace Core
