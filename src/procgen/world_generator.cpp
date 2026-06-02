#include "procgen/world_generator.h"
#include "utils/seed_hash.h"
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace Core {

namespace {
struct BoroughSeed {
    RegionId regionId;
    float normalizedX;
    float normalizedY;
    float scaleX;
    float scaleY;
};

constexpr BoroughSeed BOROUGH_SEEDS[] = {
    {RegionId::Manhattan, 0.490f, 0.160f, 2.40f, 0.85f},
    {RegionId::Manhattan, 0.485f, 0.260f, 2.40f, 0.85f},
    {RegionId::Manhattan, 0.480f, 0.360f, 2.40f, 0.85f},
    {RegionId::Manhattan, 0.475f, 0.450f, 2.40f, 0.85f},
    {RegionId::Bronx, 0.560f, 0.100f, 1.15f, 1.0f},
    {RegionId::Bronx, 0.655f, 0.120f, 1.15f, 1.0f},
    {RegionId::Queens, 0.660f, 0.340f, 1.0f, 1.0f},
    {RegionId::Queens, 0.770f, 0.300f, 1.0f, 1.0f},
    {RegionId::Queens, 0.860f, 0.400f, 1.0f, 1.0f},
    {RegionId::Queens, 0.800f, 0.520f, 1.0f, 1.0f},
    {RegionId::Queens, 0.900f, 0.500f, 1.0f, 1.0f},
    {RegionId::Queens, 0.720f, 0.450f, 1.0f, 1.0f},
    {RegionId::Queens, 0.840f, 0.600f, 1.0f, 1.0f},
    {RegionId::Brooklyn, 0.520f, 0.620f, 1.0f, 1.0f},
    {RegionId::Brooklyn, 0.605f, 0.720f, 1.0f, 1.0f},
    {RegionId::Brooklyn, 0.505f, 0.560f, 1.0f, 1.0f},
    {RegionId::StatenIsland, 0.220f, 0.760f, 1.0f, 1.0f},
    {RegionId::StatenIsland, 0.160f, 0.830f, 1.0f, 1.0f},
    {RegionId::NewJersey, 0.160f, 0.180f, 1.0f, 1.0f},
    {RegionId::NewJersey, 0.120f, 0.400f, 1.0f, 1.0f},
    {RegionId::NewJersey, 0.180f, 0.580f, 1.0f, 1.0f},
    {RegionId::NewJersey, 0.100f, 0.720f, 1.0f, 1.0f},
    {RegionId::NewJersey, 0.205f, 0.060f, 1.0f, 1.0f},
    {RegionId::NewJersey, 0.300f, 0.300f, 1.0f, 1.0f},
};
constexpr int32_t BOROUGH_SEED_COUNT = 24;

constexpr float WARP_STRENGTH = 0.055f;
constexpr float WARP_FREQUENCY = 3.0f;
constexpr float RIVER_CHANNEL = 0.020f;
constexpr float COAST_BASE = 0.880f;
constexpr float COAST_NOISE = 0.030f;
constexpr float FRAME_MARGIN = 0.014f;
constexpr int32_t AVENUE_SPACING = 11;
constexpr int32_t STREET_SPACING = 5;
constexpr int32_t PLAZA_SPACING = 33;
constexpr uint64_t WARP_STREAM_X = 0xA17C3D9BULL;
constexpr uint64_t WARP_STREAM_Y = 0x5E2B81F3ULL;

float smoothInterpolant(float t) {
    return t * t * (3.0f - 2.0f * t);
}

float linearMix(float a, float b, float t) {
    return a + (b - a) * t;
}

float latticeValue(uint64_t streamSeed, int32_t latticeX, int32_t latticeY) {
    const uint32_t hashed = Utils::hashSeedMix(streamSeed, latticeX, latticeY);
    return static_cast<float>(hashed & 0xFFFFFFu) / static_cast<float>(0xFFFFFFu);
}

float valueNoise(uint64_t streamSeed, float sampleX, float sampleY) {
    const int32_t cellX = static_cast<int32_t>(std::floor(sampleX));
    const int32_t cellY = static_cast<int32_t>(std::floor(sampleY));
    const float fractionX = smoothInterpolant(sampleX - static_cast<float>(cellX));
    const float fractionY = smoothInterpolant(sampleY - static_cast<float>(cellY));
    const float corner00 = latticeValue(streamSeed, cellX, cellY);
    const float corner10 = latticeValue(streamSeed, cellX + 1, cellY);
    const float corner01 = latticeValue(streamSeed, cellX, cellY + 1);
    const float corner11 = latticeValue(streamSeed, cellX + 1, cellY + 1);
    return linearMix(linearMix(corner00, corner10, fractionX), linearMix(corner01, corner11, fractionX), fractionY);
}
} // namespace

void WorldGenerator::generate(const WorldConfig& worldConfig, ChunkStore& chunkStore, uint64_t inputSeed) {
    worldSeed = inputSeed;
    worldWidth = worldConfig.WORLD_WIDTH_TILES;
    worldHeight = worldConfig.WORLD_HEIGHT_TILES;
    passBoroughs(chunkStore);
    passStreets(chunkStore);
    passParks(chunkStore);
    passElevation(chunkStore);
}

float WorldGenerator::warpNoise(float sampleX, float sampleY, uint64_t streamSeed) const {
    return valueNoise(worldSeed ^ streamSeed, sampleX * WARP_FREQUENCY, sampleY * WARP_FREQUENCY) - 0.5f;
}

bool WorldGenerator::isWaterBoundary(RegionId nearest, RegionId second) const {
    if (nearest == second) {
        return false;
    }
    const bool isLongIslandPair = (nearest == RegionId::Brooklyn && second == RegionId::Queens)
        || (nearest == RegionId::Queens && second == RegionId::Brooklyn);
    return !isLongIslandPair;
}

BoroughResult WorldGenerator::classifyBorough(float normalizedX, float normalizedY) const {
    const float warpedX = normalizedX + warpNoise(normalizedX, normalizedY, WARP_STREAM_X) * WARP_STRENGTH;
    const float warpedY = normalizedY + warpNoise(normalizedX, normalizedY, WARP_STREAM_Y) * WARP_STRENGTH;
    float bestDistance = 1e9f;
    float secondDistance = 1e9f;
    RegionId bestRegion = RegionId::None;
    RegionId secondRegion = RegionId::None;
    for (int32_t index = 0; index < BOROUGH_SEED_COUNT; ++index) {
        const BoroughSeed& seed = BOROUGH_SEEDS[index];
        const float deltaX = (warpedX - seed.normalizedX) * seed.scaleX;
        const float deltaY = (warpedY - seed.normalizedY) * seed.scaleY;
        const float distance = deltaX * deltaX + deltaY * deltaY;
        if (distance < bestDistance) {
            secondDistance = bestDistance;
            secondRegion = bestRegion;
            bestDistance = distance;
            bestRegion = seed.regionId;
            continue;
        }
        if (distance < secondDistance && seed.regionId != bestRegion) {
            secondDistance = distance;
            secondRegion = seed.regionId;
        }
    }
    BoroughResult result;
    result.nearest = bestRegion;
    result.second = secondRegion;
    result.boundaryGap = std::sqrt(secondDistance) - std::sqrt(bestDistance);
    return result;
}

bool WorldGenerator::isOceanTile(float normalizedX, float normalizedY) const {
    if (normalizedX < FRAME_MARGIN || normalizedX > 1.0f - FRAME_MARGIN) {
        return true;
    }
    if (normalizedY < FRAME_MARGIN || normalizedY > 1.0f - FRAME_MARGIN) {
        return true;
    }
    const float coastNoise = warpNoise(normalizedX, normalizedY, 0x33AA55ULL) * 2.0f;
    const float southCoast = COAST_BASE + coastNoise * COAST_NOISE;
    if (normalizedY > southCoast) {
        return true;
    }
    const float soundEdge = normalizedX - normalizedY + coastNoise * COAST_NOISE;
    if (soundEdge > 0.62f && normalizedY < 0.30f) {
        return true;
    }
    return false;
}

void WorldGenerator::passBoroughs(ChunkStore& chunkStore) {
    const float widthNorm = static_cast<float>(worldWidth - 1);
    const float heightNorm = static_cast<float>(worldHeight - 1);
    for (int32_t y = 0; y < worldHeight; ++y) {
        for (int32_t x = 0; x < worldWidth; ++x) {
            const float normalizedX = static_cast<float>(x) / widthNorm;
            const float normalizedY = static_cast<float>(y) / heightNorm;
            const WorldCoord coord{x, y};
            if (isOceanTile(normalizedX, normalizedY)) {
                chunkStore.setTerrainAt(coord, TerrainId::Water);
                chunkStore.setRegionAt(coord, RegionId::None);
                continue;
            }
            const BoroughResult borough = classifyBorough(normalizedX, normalizedY);
            if (isWaterBoundary(borough.nearest, borough.second) && borough.boundaryGap < RIVER_CHANNEL) {
                chunkStore.setTerrainAt(coord, TerrainId::Water);
                chunkStore.setRegionAt(coord, RegionId::None);
                continue;
            }
            chunkStore.setTerrainAt(coord, TerrainId::Building);
            chunkStore.setRegionAt(coord, borough.nearest);
        }
    }
}

void WorldGenerator::passStreets(ChunkStore& chunkStore) {
    for (int32_t y = 0; y < worldHeight; ++y) {
        for (int32_t x = 0; x < worldWidth; ++x) {
            const WorldCoord coord{x, y};
            if (chunkStore.getTerrainAt(coord) != TerrainId::Building) {
                continue;
            }
            const bool isAvenue = (x % AVENUE_SPACING) == 0;
            const bool isStreet = (y % STREET_SPACING) == 0;
            if (isAvenue || isStreet) {
                chunkStore.setTerrainAt(coord, TerrainId::Road);
            }
        }
    }
}

bool WorldGenerator::isCentralParkTile(int32_t x, int32_t y) const {
    const float normalizedX = static_cast<float>(x) / static_cast<float>(worldWidth - 1);
    const float normalizedY = static_cast<float>(y) / static_cast<float>(worldHeight - 1);
    const bool inHorizontalBand = normalizedX > 0.452f && normalizedX < 0.500f;
    const bool inVerticalBand = normalizedY > 0.235f && normalizedY < 0.345f;
    return inHorizontalBand && inVerticalBand;
}

void WorldGenerator::passParks(ChunkStore& chunkStore) {
    for (int32_t y = 0; y < worldHeight; ++y) {
        for (int32_t x = 0; x < worldWidth; ++x) {
            const WorldCoord coord{x, y};
            const TerrainId terrainId = chunkStore.getTerrainAt(coord);
            if (terrainId != TerrainId::Building && terrainId != TerrainId::Road) {
                continue;
            }
            if (chunkStore.getRegionAt(coord) == RegionId::Manhattan && isCentralParkTile(x, y)) {
                chunkStore.setTerrainAt(coord, TerrainId::Park);
                continue;
            }
            if (terrainId != TerrainId::Building) {
                continue;
            }
            const uint32_t hash = Utils::hashSeedMix(worldSeed, x / 7, y / 7);
            if ((hash % 41u) == 0u) {
                chunkStore.setTerrainAt(coord, TerrainId::Park);
                continue;
            }
            if ((x % PLAZA_SPACING) == 0 && (y % PLAZA_SPACING) == 0) {
                chunkStore.setTerrainAt(coord, TerrainId::Plaza);
            }
        }
    }
}

void WorldGenerator::passElevation(ChunkStore& chunkStore) {
    for (int32_t y = 0; y < worldHeight; ++y) {
        for (int32_t x = 0; x < worldWidth; ++x) {
            const WorldCoord coord{x, y};
            const TerrainId terrainId = chunkStore.getTerrainAt(coord);
            if (terrainId == TerrainId::Water) {
                chunkStore.setElevationAt(coord, 0);
                continue;
            }
            if (terrainId == TerrainId::Park || terrainId == TerrainId::Plaza) {
                chunkStore.setElevationAt(coord, 4);
                continue;
            }
            if (terrainId == TerrainId::Road) {
                chunkStore.setElevationAt(coord, 2);
                continue;
            }
            int16_t baseHeight = 16;
            switch (chunkStore.getRegionAt(coord)) {
            case RegionId::Manhattan: baseHeight = 70; break;
            case RegionId::Brooklyn: baseHeight = 30; break;
            case RegionId::Queens: baseHeight = 28; break;
            case RegionId::Bronx: baseHeight = 34; break;
            case RegionId::StatenIsland: baseHeight = 18; break;
            case RegionId::NewJersey: baseHeight = 22; break;
            default: baseHeight = 16; break;
            }
            const uint32_t hash = Utils::hashSeedMix(worldSeed, x, y);
            chunkStore.setElevationAt(coord, static_cast<int16_t>(baseHeight + static_cast<int16_t>(hash % 16u)));
        }
    }
}

} // namespace Core
