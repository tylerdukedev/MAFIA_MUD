#include "procgen/world_generator.h"
#include "utils/seed_hash.h"
#include <cmath>
#include <cstdint>

namespace Core {

namespace {
constexpr float WORLD_NORMALIZER = 511.0f;

struct RegionAnchor {
    RegionId regionId;
    float normalizedX;
    float normalizedY;
};

constexpr RegionAnchor REGION_ANCHORS[] = {
    {RegionId::Manhattan, 0.350f, 0.45f},
    {RegionId::Brooklyn, 0.445f, 0.32f},
    {RegionId::Queens, 0.555f, 0.42f},
    {RegionId::Bronx, 0.400f, 0.665f},
    {RegionId::StatenIsland, 0.235f, 0.125f},
    {RegionId::NewJersey, 0.185f, 0.42f},
};
constexpr int32_t REGION_ANCHOR_COUNT = 6;

float getHudsonEdge(float normalizedY) {
    return 0.292f + 0.006f * std::sin(normalizedY * 14.0f);
}

float getEastRiverWestEdge(float normalizedY) {
    return 0.358f + 0.004f * std::sin(normalizedY * 11.0f);
}

float getEastRiverEastEdge(float normalizedY) {
    return 0.382f + 0.004f * std::cos(normalizedY * 10.0f);
}

bool isInsideManhattan(float normalizedX, float normalizedY) {
    if (normalizedY < 0.18f || normalizedY > 0.72f) {
        return false;
    }
    const float centerX = 0.350f + 0.004f * std::sin(normalizedY * 9.0f);
    const float halfWidth = 0.014f + 0.006f * std::sin((normalizedY - 0.18f) * 6.0f);
    return std::abs(normalizedX - centerX) <= halfWidth;
}

bool isInsideStatenIsland(float normalizedX, float normalizedY) {
    const float deltaX = (normalizedX - 0.235f) / 0.075f;
    const float deltaY = (normalizedY - 0.125f) / 0.055f;
    return (deltaX * deltaX + deltaY * deltaY) <= 1.0f;
}

bool isInsideBronx(float normalizedX, float normalizedY) {
    if (normalizedY < 0.56f || normalizedY > 0.82f) {
        return false;
    }
    if (normalizedX < 0.335f || normalizedX > 0.54f) {
        return false;
    }
    const float southEdge = 0.56f + 0.015f * std::sin(normalizedX * 12.0f);
    return normalizedY >= southEdge;
}

bool isInsideBrooklyn(float normalizedX, float normalizedY) {
    if (normalizedY < 0.14f || normalizedY > 0.46f) {
        return false;
    }
    if (normalizedX < getEastRiverEastEdge(normalizedY) || normalizedX > 0.54f) {
        return false;
    }
    const float northBay = 0.44f - 0.04f * std::sin(normalizedX * 8.0f);
    return normalizedY <= northBay;
}

bool isInsideQueens(float normalizedX, float normalizedY) {
    if (normalizedY < 0.24f || normalizedY > 0.62f) {
        return false;
    }
    if (normalizedX < 0.415f || normalizedX > 0.70f) {
        return false;
    }
    if (isInsideBrooklyn(normalizedX, normalizedY)) {
        return false;
    }
    if (isInsideBronx(normalizedX, normalizedY)) {
        return false;
    }
    return true;
}

bool isInsideNewJersey(float normalizedX, float normalizedY) {
    if (normalizedX >= getHudsonEdge(normalizedY)) {
        return false;
    }
    if (normalizedY < 0.05f || normalizedY > 0.92f) {
        return false;
    }
    return normalizedX >= 0.10f;
}
} // namespace

void WorldGenerator::generate(const WorldConfig& worldConfig, ChunkStore& chunkStore, uint64_t inputSeed) {
    worldSeed = inputSeed;
    passGeography(worldConfig, chunkStore);
    passRegions(worldConfig, chunkStore);
    passTerrain(worldConfig, chunkStore);
}

bool WorldGenerator::isWaterTile(float normalizedX, float normalizedY) const {
    if (normalizedX > 0.695f + 0.035f * std::sin(normalizedY * 7.0f)) {
        return true;
    }
    if (normalizedX < 0.08f) {
        return true;
    }
    const float hudsonEdge = getHudsonEdge(normalizedY);
    if (normalizedX < hudsonEdge && normalizedY > 0.04f && normalizedY < 0.93f) {
        return true;
    }
    const float eastRiverWest = getEastRiverWestEdge(normalizedY);
    const float eastRiverEast = getEastRiverEastEdge(normalizedY);
    if (normalizedX > eastRiverWest && normalizedX < eastRiverEast && normalizedY > 0.12f && normalizedY < 0.74f) {
        return true;
    }
    if (normalizedY < 0.105f && normalizedX > 0.26f && normalizedX < 0.62f) {
        return true;
    }
    if (normalizedY < 0.19f && normalizedX > 0.30f && normalizedX < 0.44f) {
        return true;
    }
    if (isInsideStatenIsland(normalizedX, normalizedY)) {
        return false;
    }
    if (isInsideManhattan(normalizedX, normalizedY)) {
        return false;
    }
    if (normalizedY < 0.22f && normalizedX > 0.24f && normalizedX < 0.34f) {
        const float deltaX = normalizedX - 0.235f;
        const float deltaY = normalizedY - 0.125f;
        const float distanceSquared = deltaX * deltaX + deltaY * deltaY;
        if (distanceSquared > 0.003f) {
            return true;
        }
    }
    if (normalizedX > 0.58f && normalizedY > 0.50f && normalizedY < 0.72f) {
        return true;
    }
    return false;
}

RegionId WorldGenerator::pickRegionForLand(float normalizedX, float normalizedY) const {
    if (isInsideManhattan(normalizedX, normalizedY)) {
        return RegionId::Manhattan;
    }
    if (isInsideStatenIsland(normalizedX, normalizedY)) {
        return RegionId::StatenIsland;
    }
    if (isInsideBronx(normalizedX, normalizedY)) {
        return RegionId::Bronx;
    }
    if (isInsideBrooklyn(normalizedX, normalizedY)) {
        return RegionId::Brooklyn;
    }
    if (isInsideQueens(normalizedX, normalizedY)) {
        return RegionId::Queens;
    }
    if (isInsideNewJersey(normalizedX, normalizedY)) {
        return RegionId::NewJersey;
    }
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
