#include "procgen/world_generator.h"
#include "utils/seed_hash.h"
#include <cmath>
#include <cstdint>

namespace Core {

namespace {
struct RegionAnchor {
    RegionId regionId;
    float normalizedX;
    float normalizedY;
};

constexpr RegionAnchor REGION_ANCHORS[] = {
    {RegionId::Manhattan, 0.345f, 0.42f},
    {RegionId::Brooklyn, 0.415f, 0.38f},
    {RegionId::Queens, 0.52f, 0.44f},
    {RegionId::Bronx, 0.375f, 0.62f},
    {RegionId::StatenIsland, 0.265f, 0.155f},
    {RegionId::NewJersey, 0.20f, 0.42f},
};
constexpr int32_t REGION_ANCHOR_COUNT = 6;
constexpr float WORLD_NORMALIZER = 511.0f;
} // namespace

void WorldGenerator::generate(const WorldConfig& worldConfig, ChunkStore& chunkStore, uint64_t inputSeed) {
    worldSeed = inputSeed;
    passGeography(worldConfig, chunkStore);
    passRegions(worldConfig, chunkStore);
    passTerrain(worldConfig, chunkStore);
}

bool WorldGenerator::isWaterTile(float normalizedX, float normalizedY) const {
    if (normalizedX > 0.70f) {
        return true;
    }
    if (normalizedX < 0.14f) {
        return true;
    }
    const float hudsonEdge = 0.28f + 0.015f * std::sin(normalizedY * 12.0f);
    if (normalizedX < hudsonEdge && normalizedY > 0.08f && normalizedY < 0.88f) {
        return true;
    }
    if (normalizedX > 0.355f && normalizedX < 0.38f && normalizedY > 0.12f && normalizedY < 0.72f) {
        return true;
    }
    if (normalizedY < 0.11f && normalizedX > 0.20f && normalizedX < 0.65f) {
        return true;
    }
    if (normalizedY < 0.18f && normalizedX > 0.22f && normalizedX < 0.35f) {
        return true;
    }
    const float deltaX = normalizedX - 0.26f;
    const float deltaY = normalizedY - 0.16f;
    const float distanceSquared = deltaX * deltaX + deltaY * deltaY;
    if (distanceSquared > 0.008f && distanceSquared < 0.025f && normalizedX < 0.32f && normalizedY < 0.24f) {
        return true;
    }
    return false;
}

RegionId WorldGenerator::pickRegionForLand(float normalizedX, float normalizedY) const {
    RegionId closestRegion = RegionId::None;
    float closestDistanceSquared = 999.0f;
    for (int32_t index = 0; index < REGION_ANCHOR_COUNT; ++index) {
        const RegionAnchor& anchor = REGION_ANCHORS[index];
        const float deltaX = normalizedX - anchor.normalizedX;
        const float deltaY = normalizedY - anchor.normalizedY;
        const float distanceSquared = deltaX * deltaX + deltaY * deltaY;
        if (distanceSquared < closestDistanceSquared) {
            closestDistanceSquared = distanceSquared;
            closestRegion = anchor.regionId;
        }
    }
    return closestRegion;
}

void WorldGenerator::passGeography(const WorldConfig& worldConfig, ChunkStore& chunkStore) {
    for (int32_t y = 0; y < worldConfig.WORLD_HEIGHT_TILES; ++y) {
        for (int32_t x = 0; x < worldConfig.WORLD_WIDTH_TILES; ++x) {
            WorldCoord coord{x, y};
            const float normalizedX = static_cast<float>(x) / WORLD_NORMALIZER;
            const float normalizedY = static_cast<float>(y) / WORLD_NORMALIZER;
            if (isWaterTile(normalizedX, normalizedY)) {
                chunkStore.setTerrainAt(coord, TerrainId::Water);
                chunkStore.setRegionAt(coord, RegionId::None);
            }
        }
    }
}

void WorldGenerator::passRegions(const WorldConfig& worldConfig, ChunkStore& chunkStore) {
    for (int32_t y = 0; y < worldConfig.WORLD_HEIGHT_TILES; ++y) {
        for (int32_t x = 0; x < worldConfig.WORLD_WIDTH_TILES; ++x) {
            WorldCoord coord{x, y};
            if (chunkStore.getTerrainAt(coord) == TerrainId::Water) {
                continue;
            }
            const float normalizedX = static_cast<float>(x) / WORLD_NORMALIZER;
            const float normalizedY = static_cast<float>(y) / WORLD_NORMALIZER;
            const RegionId regionId = pickRegionForLand(normalizedX, normalizedY);
            chunkStore.setRegionAt(coord, regionId);
        }
    }
}

void WorldGenerator::passTerrain(const WorldConfig& worldConfig, ChunkStore& chunkStore) {
    for (int32_t y = 0; y < worldConfig.WORLD_HEIGHT_TILES; ++y) {
        for (int32_t x = 0; x < worldConfig.WORLD_WIDTH_TILES; ++x) {
            WorldCoord coord{x, y};
            if (chunkStore.getTerrainAt(coord) == TerrainId::Water) {
                chunkStore.setElevationAt(coord, 0);
                continue;
            }
            chunkStore.setTerrainAt(coord, TerrainId::Land);
            const uint32_t hash = Utils::hashSeedMix(worldSeed, x, y);
            const int16_t elevation = static_cast<int16_t>(5 + (hash % 40U));
            chunkStore.setElevationAt(coord, elevation);
        }
    }
}

} // namespace Core
