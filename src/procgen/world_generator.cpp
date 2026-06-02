#include "procgen/world_generator.h"
#include "utils/seed_hash.h"
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace Core {

namespace {
struct MapSeed {
    RegionId regionId;
    float normalizedX;
    float normalizedY;
    float scaleX;
    float scaleY;
    bool isWater;
};

constexpr MapSeed MAP_SEEDS[] = {
    {RegionId::Manhattan, 0.548f, 0.150f, 3.40f, 0.70f, false},
    {RegionId::Manhattan, 0.540f, 0.225f, 3.40f, 0.70f, false},
    {RegionId::Manhattan, 0.531f, 0.300f, 3.40f, 0.70f, false},
    {RegionId::Manhattan, 0.522f, 0.375f, 3.40f, 0.70f, false},
    {RegionId::Manhattan, 0.514f, 0.450f, 3.40f, 0.70f, false},
    {RegionId::Manhattan, 0.508f, 0.515f, 3.40f, 0.70f, false},
    {RegionId::Bronx, 0.640f, 0.080f, 1.0f, 1.0f, false},
    {RegionId::Bronx, 0.710f, 0.110f, 1.0f, 1.0f, false},
    {RegionId::Bronx, 0.660f, 0.160f, 1.0f, 1.0f, false},
    {RegionId::Queens, 0.770f, 0.200f, 1.0f, 1.0f, false},
    {RegionId::Queens, 0.850f, 0.250f, 1.0f, 1.0f, false},
    {RegionId::Queens, 0.720f, 0.280f, 1.0f, 1.0f, false},
    {RegionId::Queens, 0.810f, 0.320f, 1.0f, 1.0f, false},
    {RegionId::Queens, 0.855f, 0.340f, 1.0f, 1.0f, false},
    {RegionId::Queens, 0.740f, 0.380f, 1.0f, 1.0f, false},
    {RegionId::Queens, 0.840f, 0.440f, 1.0f, 1.0f, false},
    {RegionId::Queens, 0.780f, 0.500f, 1.0f, 1.0f, false},
    {RegionId::Queens, 0.840f, 0.580f, 1.0f, 1.0f, false},
    {RegionId::Queens, 0.720f, 0.460f, 1.0f, 1.0f, false},
    {RegionId::Brooklyn, 0.500f, 0.620f, 1.0f, 1.0f, false},
    {RegionId::Brooklyn, 0.580f, 0.700f, 1.0f, 1.0f, false},
    {RegionId::Brooklyn, 0.480f, 0.580f, 1.0f, 1.0f, false},
    {RegionId::Brooklyn, 0.560f, 0.660f, 1.0f, 1.0f, false},
    {RegionId::StatenIsland, 0.220f, 0.740f, 1.0f, 1.0f, false},
    {RegionId::StatenIsland, 0.160f, 0.820f, 1.0f, 1.0f, false},
    {RegionId::StatenIsland, 0.280f, 0.800f, 1.0f, 1.0f, false},
    {RegionId::NewJersey, 0.110f, 0.160f, 1.0f, 1.0f, false},
    {RegionId::NewJersey, 0.070f, 0.400f, 1.0f, 1.0f, false},
    {RegionId::NewJersey, 0.110f, 0.600f, 1.0f, 1.0f, false},
    {RegionId::NewJersey, 0.170f, 0.780f, 1.0f, 1.0f, false},
    {RegionId::NewJersey, 0.150f, 0.050f, 1.0f, 1.0f, false},
    {RegionId::NewJersey, 0.060f, 0.280f, 1.0f, 1.0f, false},
    {RegionId::NewJersey, 0.100f, 0.500f, 1.0f, 1.0f, false},
    {RegionId::NewJersey, 0.300f, 0.250f, 1.0f, 1.0f, false},
    {RegionId::NewJersey, 0.340f, 0.560f, 1.0f, 1.0f, false},
    {RegionId::NewJersey, 0.310f, 0.700f, 1.0f, 1.0f, false},
    {RegionId::Westchester, 0.430f, 0.030f, 1.0f, 1.0f, false},
    {RegionId::Westchester, 0.490f, 0.040f, 1.0f, 1.0f, false},
    {RegionId::Westchester, 0.560f, 0.020f, 1.0f, 1.0f, false},
    {RegionId::Westchester, 0.320f, 0.040f, 1.0f, 1.0f, false},
    {RegionId::Westchester, 0.700f, 0.030f, 1.0f, 1.0f, false},
    {RegionId::Westchester, 0.800f, 0.060f, 1.0f, 1.0f, false},
    {RegionId::LongIsland, 0.920f, 0.250f, 1.0f, 1.0f, false},
    {RegionId::LongIsland, 0.940f, 0.450f, 1.0f, 1.0f, false},
    {RegionId::LongIsland, 0.915f, 0.150f, 1.0f, 1.0f, false},
    {RegionId::LongIsland, 0.950f, 0.350f, 1.0f, 1.0f, false},
    {RegionId::LongIsland, 0.930f, 0.580f, 1.0f, 1.0f, false},
    {RegionId::LongIsland, 0.955f, 0.680f, 1.0f, 1.0f, false},
    {RegionId::None, 0.430f, 0.620f, 1.0f, 1.0f, true},
    {RegionId::None, 0.400f, 0.690f, 1.0f, 1.0f, true},
    {RegionId::None, 0.470f, 0.575f, 1.0f, 1.0f, true},
    {RegionId::None, 0.385f, 0.715f, 1.0f, 1.0f, true},
    {RegionId::None, 0.300f, 0.960f, 1.0f, 1.0f, true},
    {RegionId::None, 0.620f, 0.965f, 1.0f, 1.0f, true},
    {RegionId::None, 0.150f, 0.945f, 1.0f, 1.0f, true},
    {RegionId::None, 0.900f, 0.800f, 1.0f, 1.0f, true},
};
constexpr int32_t MAP_SEED_COUNT = 56;

constexpr float WARP_STRENGTH = 0.050f;
constexpr float WARP_FREQUENCY = 3.0f;
constexpr float RIVER_CHANNEL = 0.012f;
constexpr float ATLANTIC_BASE = 0.945f;
constexpr float ATLANTIC_NOISE = 0.035f;
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

bool WorldGenerator::isMainlandRegion(RegionId regionId) const {
    return regionId == RegionId::NewJersey || regionId == RegionId::Westchester || regionId == RegionId::LongIsland;
}

bool WorldGenerator::isWaterBoundary(RegionId nearest, RegionId second) const {
    if (nearest == second) {
        return false;
    }
    const bool brooklynQueens = (nearest == RegionId::Brooklyn && second == RegionId::Queens)
        || (nearest == RegionId::Queens && second == RegionId::Brooklyn);
    const bool queensLongIsland = (nearest == RegionId::Queens && second == RegionId::LongIsland)
        || (nearest == RegionId::LongIsland && second == RegionId::Queens);
    const bool bronxWestchester = (nearest == RegionId::Bronx && second == RegionId::Westchester)
        || (nearest == RegionId::Westchester && second == RegionId::Bronx);
    if (brooklynQueens || queensLongIsland || bronxWestchester) {
        return false;
    }
    return true;
}

BoroughResult WorldGenerator::classifyBorough(float normalizedX, float normalizedY) const {
    const float warpedX = normalizedX + warpNoise(normalizedX, normalizedY, WARP_STREAM_X) * WARP_STRENGTH;
    const float warpedY = normalizedY + warpNoise(normalizedX, normalizedY, WARP_STREAM_Y) * WARP_STRENGTH;
    float bestAnyDistance = 1e9f;
    bool bestAnyIsWater = false;
    float bestLandDistance = 1e9f;
    float secondLandDistance = 1e9f;
    RegionId bestLandRegion = RegionId::None;
    RegionId secondLandRegion = RegionId::None;
    for (int32_t index = 0; index < MAP_SEED_COUNT; ++index) {
        const MapSeed& seed = MAP_SEEDS[index];
        const float deltaX = (warpedX - seed.normalizedX) * seed.scaleX;
        const float deltaY = (warpedY - seed.normalizedY) * seed.scaleY;
        const float distance = deltaX * deltaX + deltaY * deltaY;
        if (distance < bestAnyDistance) {
            bestAnyDistance = distance;
            bestAnyIsWater = seed.isWater;
        }
        if (seed.isWater) {
            continue;
        }
        if (distance < bestLandDistance) {
            secondLandDistance = bestLandDistance;
            secondLandRegion = bestLandRegion;
            bestLandDistance = distance;
            bestLandRegion = seed.regionId;
            continue;
        }
        if (distance < secondLandDistance && seed.regionId != bestLandRegion) {
            secondLandDistance = distance;
            secondLandRegion = seed.regionId;
        }
    }
    BoroughResult result;
    result.nearest = bestLandRegion;
    result.second = secondLandRegion;
    result.boundaryGap = std::sqrt(secondLandDistance) - std::sqrt(bestLandDistance);
    result.nearestIsWater = bestAnyIsWater;
    return result;
}

bool WorldGenerator::isAtlanticTile(float normalizedX, float normalizedY) const {
    const float coastNoise = warpNoise(normalizedX, normalizedY, 0x33AA55ULL) * 2.0f;
    return normalizedY > ATLANTIC_BASE + coastNoise * ATLANTIC_NOISE;
}

void WorldGenerator::passBoroughs(ChunkStore& chunkStore) {
    const float widthNorm = static_cast<float>(worldWidth - 1);
    const float heightNorm = static_cast<float>(worldHeight - 1);
    for (int32_t y = 0; y < worldHeight; ++y) {
        for (int32_t x = 0; x < worldWidth; ++x) {
            const float normalizedX = static_cast<float>(x) / widthNorm;
            const float normalizedY = static_cast<float>(y) / heightNorm;
            const WorldCoord coord{x, y};
            const BoroughResult borough = classifyBorough(normalizedX, normalizedY);
            const bool isRiver = isWaterBoundary(borough.nearest, borough.second) && borough.boundaryGap < RIVER_CHANNEL;
            if (borough.nearestIsWater || isRiver || isAtlanticTile(normalizedX, normalizedY)) {
                chunkStore.setTerrainAt(coord, TerrainId::Water);
                chunkStore.setRegionAt(coord, RegionId::None);
                continue;
            }
            const TerrainId landTerrain = isMainlandRegion(borough.nearest) ? TerrainId::OpenLand : TerrainId::Building;
            chunkStore.setTerrainAt(coord, landTerrain);
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
    const bool inHorizontalBand = normalizedX > 0.512f && normalizedX < 0.548f;
    const bool inVerticalBand = normalizedY > 0.250f && normalizedY < 0.360f;
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
            if (terrainId == TerrainId::OpenLand) {
                chunkStore.setElevationAt(coord, 8);
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
            default: baseHeight = 16; break;
            }
            const uint32_t hash = Utils::hashSeedMix(worldSeed, x, y);
            chunkStore.setElevationAt(coord, static_cast<int16_t>(baseHeight + static_cast<int16_t>(hash % 16u)));
        }
    }
}

} // namespace Core
