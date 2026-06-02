#include "procgen/world_generator.h"
#include "utils/seed_hash.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <queue>
#include <utility>

namespace Core {

namespace {
constexpr float ELEVATION_FEATURE_SCALE = 1.0f / 112.0f;
constexpr int32_t ELEVATION_OCTAVES = 6;
constexpr float MOISTURE_FEATURE_SCALE = 1.0f / 148.0f;
constexpr int32_t MOISTURE_OCTAVES = 5;
constexpr uint64_t ELEVATION_STREAM = 0x1234ABCDULL;
constexpr uint64_t MOISTURE_STREAM = 0x9F5C2D71ULL;
constexpr uint64_t OCTAVE_STREAM_STEP = 0x9E3779B1ULL;
constexpr float EDGE_FALLOFF_SCALE = 0.86f;
constexpr float EDGE_FALLOFF_STRENGTH = 0.62f;
constexpr float SEA_LEVEL = 0.40f;
constexpr float SHALLOW_BAND = 0.05f;
constexpr float BEACH_BAND = 0.02f;
constexpr float FOREST_MOISTURE = 0.52f;
constexpr float HILL_RELATIVE = 0.44f;
constexpr float MOUNTAIN_RELATIVE = 0.68f;
constexpr float PEAK_RELATIVE = 0.88f;
constexpr float MAX_ELEVATION_UNITS = 255.0f;
constexpr int32_t RIVER_SOURCE_SPACING = 22;
constexpr float RIVER_SOURCE_RELATIVE = 0.62f;
constexpr int32_t RIVER_MAX_STEPS = 4000;
constexpr int32_t CITY_SCAN_SPACING = 34;
constexpr int32_t CITY_MIN_DISTANCE = 58;
constexpr int32_t CITY_MAX_COUNT = 14;
constexpr int32_t CITY_MIN_RADIUS = 6;
constexpr int32_t CITY_RADIUS_RANGE = 11;
constexpr int32_t CITY_STREET_SPACING = 4;
constexpr int32_t CITY_WATER_SEARCH = 9;
constexpr float CITY_MIN_SCORE = 0.20f;
constexpr int32_t ROAD_CONNECTIONS_PER_CITY = 2;
constexpr float ROAD_BASE_COST = 1.0f;
constexpr float ROAD_DIAGONAL_SCALE = 1.41421356f;
constexpr float SLOPE_PENALTY_SCALE = 24.0f;

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
    allocateFields(worldConfig);
    passElevation();
    passMoisture();
    passClassify(chunkStore);
    passRivers(chunkStore);
    passCities(chunkStore);
    passRoads(chunkStore);
}

void WorldGenerator::allocateFields(const WorldConfig& worldConfig) {
    worldWidth = worldConfig.WORLD_WIDTH_TILES;
    worldHeight = worldConfig.WORLD_HEIGHT_TILES;
    const size_t tileCount = static_cast<size_t>(worldWidth) * static_cast<size_t>(worldHeight);
    heightField.assign(tileCount, 0.0f);
    moistureField.assign(tileCount, 0.0f);
    waterField.assign(tileCount, 0u);
    pathCost.assign(tileCount, 0.0f);
    pathParent.assign(tileCount, -1);
    pathClosed.assign(tileCount, 0u);
    cities.clear();
    const uint32_t shiftHashX = Utils::hashSeedMix(worldSeed, 7, 13);
    const uint32_t shiftHashY = Utils::hashSeedMix(worldSeed, 71, 131);
    continentCenterX = worldWidth / 2 + (static_cast<int32_t>(shiftHashX % 120U) - 60);
    continentCenterY = worldHeight / 2 + (static_cast<int32_t>(shiftHashY % 120U) - 60);
}

int32_t WorldGenerator::tileIndex(int32_t x, int32_t y) const {
    return y * worldWidth + x;
}

float WorldGenerator::fractalNoise(float sampleX, float sampleY, uint64_t streamSeed, int32_t octaves) const {
    float sum = 0.0f;
    float amplitude = 1.0f;
    float frequency = 1.0f;
    float normalizer = 0.0f;
    for (int32_t octave = 0; octave < octaves; ++octave) {
        const uint64_t octaveSeed = streamSeed + static_cast<uint64_t>(octave) * OCTAVE_STREAM_STEP;
        sum += amplitude * valueNoise(octaveSeed, sampleX * frequency, sampleY * frequency);
        normalizer += amplitude;
        amplitude *= 0.5f;
        frequency *= 2.0f;
    }
    return sum / normalizer;
}

void WorldGenerator::normalizeField(std::vector<float>& field) const {
    float minValue = std::numeric_limits<float>::max();
    float maxValue = std::numeric_limits<float>::lowest();
    for (const float value : field) {
        minValue = std::min(minValue, value);
        maxValue = std::max(maxValue, value);
    }
    const float range = std::max(maxValue - minValue, 1e-5f);
    for (float& value : field) {
        value = (value - minValue) / range;
    }
}

void WorldGenerator::passElevation() {
    const float halfWidth = static_cast<float>(worldWidth) * 0.5f;
    const float halfHeight = static_cast<float>(worldHeight) * 0.5f;
    for (int32_t y = 0; y < worldHeight; ++y) {
        for (int32_t x = 0; x < worldWidth; ++x) {
            const float noiseValue = fractalNoise(static_cast<float>(x) * ELEVATION_FEATURE_SCALE, static_cast<float>(y) * ELEVATION_FEATURE_SCALE, worldSeed ^ ELEVATION_STREAM, ELEVATION_OCTAVES);
            const float normalizedX = (static_cast<float>(x - continentCenterX)) / halfWidth;
            const float normalizedY = (static_cast<float>(y - continentCenterY)) / halfHeight;
            const float radial = std::sqrt(normalizedX * normalizedX + normalizedY * normalizedY) * EDGE_FALLOFF_SCALE;
            const float falloff = radial * radial * EDGE_FALLOFF_STRENGTH;
            heightField[tileIndex(x, y)] = noiseValue - falloff;
        }
    }
    normalizeField(heightField);
}

void WorldGenerator::passMoisture() {
    for (int32_t y = 0; y < worldHeight; ++y) {
        for (int32_t x = 0; x < worldWidth; ++x) {
            moistureField[tileIndex(x, y)] = fractalNoise(static_cast<float>(x) * MOISTURE_FEATURE_SCALE, static_cast<float>(y) * MOISTURE_FEATURE_SCALE, worldSeed ^ MOISTURE_STREAM, MOISTURE_OCTAVES);
        }
    }
    normalizeField(moistureField);
}

TerrainId WorldGenerator::classifyTerrain(float normalizedHeight, float moisture) const {
    if (normalizedHeight < SEA_LEVEL - SHALLOW_BAND) {
        return TerrainId::DeepWater;
    }
    if (normalizedHeight < SEA_LEVEL) {
        return TerrainId::ShallowWater;
    }
    if (normalizedHeight < SEA_LEVEL + BEACH_BAND) {
        return TerrainId::Beach;
    }
    const float relative = (normalizedHeight - SEA_LEVEL) / (1.0f - SEA_LEVEL);
    if (relative < HILL_RELATIVE) {
        return moisture > FOREST_MOISTURE ? TerrainId::Forest : TerrainId::Grassland;
    }
    if (relative < MOUNTAIN_RELATIVE) {
        return TerrainId::Hills;
    }
    if (relative < PEAK_RELATIVE) {
        return TerrainId::Mountain;
    }
    return TerrainId::Peak;
}

RegionId WorldGenerator::regionForTerrain(TerrainId terrainId) const {
    switch (terrainId) {
    case TerrainId::DeepWater:
    case TerrainId::ShallowWater: return RegionId::Ocean;
    case TerrainId::River:
    case TerrainId::Beach: return RegionId::Coast;
    case TerrainId::Grassland: return RegionId::Plains;
    case TerrainId::Forest: return RegionId::Forest;
    case TerrainId::Hills: return RegionId::Highlands;
    case TerrainId::Mountain:
    case TerrainId::Peak: return RegionId::Mountains;
    case TerrainId::City:
    case TerrainId::Road: return RegionId::Settlement;
    default: return RegionId::None;
    }
}

void WorldGenerator::passClassify(ChunkStore& chunkStore) {
    for (int32_t y = 0; y < worldHeight; ++y) {
        for (int32_t x = 0; x < worldWidth; ++x) {
            const int32_t fieldIndex = tileIndex(x, y);
            const float normalizedHeight = heightField[fieldIndex];
            const TerrainId terrainId = classifyTerrain(normalizedHeight, moistureField[fieldIndex]);
            const WorldCoord coord{x, y};
            chunkStore.setTerrainAt(coord, terrainId);
            chunkStore.setRegionAt(coord, regionForTerrain(terrainId));
            chunkStore.setElevationAt(coord, static_cast<int16_t>(normalizedHeight * MAX_ELEVATION_UNITS));
            const bool isWater = terrainId == TerrainId::DeepWater || terrainId == TerrainId::ShallowWater;
            waterField[fieldIndex] = isWater ? 1u : 0u;
        }
    }
}

bool WorldGenerator::isWaterIndex(int32_t fieldIndex) const {
    return waterField[fieldIndex] != 0u;
}

void WorldGenerator::traceRiver(ChunkStore& chunkStore, int32_t sourceX, int32_t sourceY) {
    int32_t currentX = sourceX;
    int32_t currentY = sourceY;
    for (int32_t step = 0; step < RIVER_MAX_STEPS; ++step) {
        const int32_t currentIndex = tileIndex(currentX, currentY);
        if (isWaterIndex(currentIndex)) {
            return;
        }
        const WorldCoord coord{currentX, currentY};
        if (chunkStore.getTerrainAt(coord) != TerrainId::River) {
            chunkStore.setTerrainAt(coord, TerrainId::River);
            chunkStore.setRegionAt(coord, RegionId::Coast);
        }
        int32_t bestX = currentX;
        int32_t bestY = currentY;
        float bestHeight = heightField[currentIndex];
        for (int32_t offsetY = -1; offsetY <= 1; ++offsetY) {
            for (int32_t offsetX = -1; offsetX <= 1; ++offsetX) {
                const int32_t neighborX = currentX + offsetX;
                const int32_t neighborY = currentY + offsetY;
                if (neighborX < 0 || neighborX >= worldWidth || neighborY < 0 || neighborY >= worldHeight) {
                    continue;
                }
                const float neighborHeight = heightField[tileIndex(neighborX, neighborY)];
                if (neighborHeight < bestHeight) {
                    bestHeight = neighborHeight;
                    bestX = neighborX;
                    bestY = neighborY;
                }
            }
        }
        if (bestX == currentX && bestY == currentY) {
            return;
        }
        currentX = bestX;
        currentY = bestY;
    }
}

void WorldGenerator::passRivers(ChunkStore& chunkStore) {
    const float sourceHeight = SEA_LEVEL + (1.0f - SEA_LEVEL) * RIVER_SOURCE_RELATIVE;
    for (int32_t y = RIVER_SOURCE_SPACING; y < worldHeight - RIVER_SOURCE_SPACING; y += RIVER_SOURCE_SPACING) {
        for (int32_t x = RIVER_SOURCE_SPACING; x < worldWidth - RIVER_SOURCE_SPACING; x += RIVER_SOURCE_SPACING) {
            const uint32_t jitter = Utils::hashSeedMix(worldSeed ^ 0x511FE2A1ULL, x, y);
            const int32_t sourceX = x + static_cast<int32_t>(jitter % 9U) - 4;
            const int32_t sourceY = y + static_cast<int32_t>((jitter >> 8) % 9U) - 4;
            if (sourceX < 1 || sourceX >= worldWidth - 1 || sourceY < 1 || sourceY >= worldHeight - 1) {
                continue;
            }
            if (heightField[tileIndex(sourceX, sourceY)] < sourceHeight) {
                continue;
            }
            if ((jitter % 3U) != 0U) {
                continue;
            }
            traceRiver(chunkStore, sourceX, sourceY);
        }
    }
}

float WorldGenerator::scoreCitySite(const ChunkStore& chunkStore, int32_t x, int32_t y) const {
    const TerrainId terrainId = chunkStore.getTerrainAt(WorldCoord{x, y});
    const bool isBuildable = terrainId == TerrainId::Grassland || terrainId == TerrainId::Forest || terrainId == TerrainId::Beach || terrainId == TerrainId::Hills;
    if (!isBuildable) {
        return 0.0f;
    }
    float waterProximity = 0.0f;
    for (int32_t offsetY = -CITY_WATER_SEARCH; offsetY <= CITY_WATER_SEARCH; ++offsetY) {
        for (int32_t offsetX = -CITY_WATER_SEARCH; offsetX <= CITY_WATER_SEARCH; ++offsetX) {
            const int32_t neighborX = x + offsetX;
            const int32_t neighborY = y + offsetY;
            if (neighborX < 0 || neighborX >= worldWidth || neighborY < 0 || neighborY >= worldHeight) {
                continue;
            }
            const TerrainId neighborTerrain = chunkStore.getTerrainAt(WorldCoord{neighborX, neighborY});
            const bool isWaterNeighbor = neighborTerrain == TerrainId::ShallowWater || neighborTerrain == TerrainId::River;
            if (isWaterNeighbor) {
                waterProximity = 1.0f;
            }
        }
    }
    const float elevationPenalty = heightField[tileIndex(x, y)];
    const float flatlandBonus = terrainId == TerrainId::Grassland ? 0.35f : 0.15f;
    return 0.25f + waterProximity * 0.55f + flatlandBonus - elevationPenalty * 0.25f;
}

void WorldGenerator::stampCity(ChunkStore& chunkStore, const CityRecord& city) {
    const int32_t radiusSquared = city.radius * city.radius;
    for (int32_t offsetY = -city.radius; offsetY <= city.radius; ++offsetY) {
        for (int32_t offsetX = -city.radius; offsetX <= city.radius; ++offsetX) {
            if (offsetX * offsetX + offsetY * offsetY > radiusSquared) {
                continue;
            }
            const int32_t tileX = city.x + offsetX;
            const int32_t tileY = city.y + offsetY;
            if (tileX < 0 || tileX >= worldWidth || tileY < 0 || tileY >= worldHeight) {
                continue;
            }
            const WorldCoord coord{tileX, tileY};
            const TerrainId existing = chunkStore.getTerrainAt(coord);
            const bool isWater = existing == TerrainId::DeepWater || existing == TerrainId::ShallowWater || existing == TerrainId::River;
            if (isWater) {
                continue;
            }
            const bool isStreet = (tileX % CITY_STREET_SPACING == 0) || (tileY % CITY_STREET_SPACING == 0);
            chunkStore.setTerrainAt(coord, isStreet ? TerrainId::Road : TerrainId::City);
            chunkStore.setRegionAt(coord, RegionId::Settlement);
        }
    }
}

void WorldGenerator::passCities(ChunkStore& chunkStore) {
    for (int32_t y = CITY_SCAN_SPACING; y < worldHeight - CITY_SCAN_SPACING; y += CITY_SCAN_SPACING) {
        for (int32_t x = CITY_SCAN_SPACING; x < worldWidth - CITY_SCAN_SPACING; x += CITY_SCAN_SPACING) {
            if (static_cast<int32_t>(cities.size()) >= CITY_MAX_COUNT) {
                break;
            }
            const uint32_t jitter = Utils::hashSeedMix(worldSeed ^ 0xC17A9D33ULL, x, y);
            const int32_t siteX = x + static_cast<int32_t>(jitter % 17U) - 8;
            const int32_t siteY = y + static_cast<int32_t>((jitter >> 8) % 17U) - 8;
            if (siteX < CITY_MIN_RADIUS || siteX >= worldWidth - CITY_MIN_RADIUS || siteY < CITY_MIN_RADIUS || siteY >= worldHeight - CITY_MIN_RADIUS) {
                continue;
            }
            if (scoreCitySite(chunkStore, siteX, siteY) < CITY_MIN_SCORE) {
                continue;
            }
            bool tooClose = false;
            for (const CityRecord& placed : cities) {
                const int32_t deltaX = placed.x - siteX;
                const int32_t deltaY = placed.y - siteY;
                if (deltaX * deltaX + deltaY * deltaY < CITY_MIN_DISTANCE * CITY_MIN_DISTANCE) {
                    tooClose = true;
                }
            }
            if (tooClose) {
                continue;
            }
            const int32_t radius = CITY_MIN_RADIUS + static_cast<int32_t>((jitter >> 16) % static_cast<uint32_t>(CITY_RADIUS_RANGE));
            const CityRecord city{siteX, siteY, radius};
            cities.push_back(city);
            stampCity(chunkStore, city);
        }
    }
}

float WorldGenerator::computeStepCost(const ChunkStore& chunkStore, int32_t toX, int32_t toY, bool isDiagonal) const {
    const TerrainId terrainId = chunkStore.getTerrainAt(WorldCoord{toX, toY});
    float terrainPenalty = 0.0f;
    switch (terrainId) {
    case TerrainId::DeepWater: terrainPenalty = 40.0f; break;
    case TerrainId::ShallowWater: terrainPenalty = 8.0f; break;
    case TerrainId::River: terrainPenalty = 5.0f; break;
    case TerrainId::Peak: terrainPenalty = 22.0f; break;
    case TerrainId::Mountain: terrainPenalty = 9.0f; break;
    case TerrainId::Hills: terrainPenalty = 3.0f; break;
    case TerrainId::Forest: terrainPenalty = 2.0f; break;
    case TerrainId::City:
    case TerrainId::Road: terrainPenalty = -0.4f; break;
    default: terrainPenalty = 0.5f; break;
    }
    const float base = isDiagonal ? ROAD_BASE_COST * ROAD_DIAGONAL_SCALE : ROAD_BASE_COST;
    return std::max(0.05f, base + terrainPenalty);
}

void WorldGenerator::carveRoad(ChunkStore& chunkStore, const CityRecord& cityA, const CityRecord& cityB) {
    std::fill(pathCost.begin(), pathCost.end(), std::numeric_limits<float>::max());
    std::fill(pathParent.begin(), pathParent.end(), -1);
    std::fill(pathClosed.begin(), pathClosed.end(), 0u);
    const int32_t startIndex = tileIndex(cityA.x, cityA.y);
    const int32_t goalIndex = tileIndex(cityB.x, cityB.y);
    const float goalX = static_cast<float>(cityB.x);
    const float goalY = static_cast<float>(cityB.y);
    using OpenNode = std::pair<float, int32_t>;
    std::priority_queue<OpenNode, std::vector<OpenNode>, std::greater<OpenNode>> openSet;
    pathCost[startIndex] = 0.0f;
    openSet.push({0.0f, startIndex});
    while (!openSet.empty()) {
        const int32_t currentIndex = openSet.top().second;
        openSet.pop();
        if (currentIndex == goalIndex) {
            break;
        }
        if (pathClosed[currentIndex] != 0u) {
            continue;
        }
        pathClosed[currentIndex] = 1u;
        const int32_t currentX = currentIndex % worldWidth;
        const int32_t currentY = currentIndex / worldWidth;
        for (int32_t offsetY = -1; offsetY <= 1; ++offsetY) {
            for (int32_t offsetX = -1; offsetX <= 1; ++offsetX) {
                if (offsetX == 0 && offsetY == 0) {
                    continue;
                }
                const int32_t neighborX = currentX + offsetX;
                const int32_t neighborY = currentY + offsetY;
                if (neighborX < 0 || neighborX >= worldWidth || neighborY < 0 || neighborY >= worldHeight) {
                    continue;
                }
                const int32_t neighborIndex = tileIndex(neighborX, neighborY);
                if (pathClosed[neighborIndex] != 0u) {
                    continue;
                }
                const bool isDiagonal = offsetX != 0 && offsetY != 0;
                const float tentative = pathCost[currentIndex] + computeStepCost(chunkStore, neighborX, neighborY, isDiagonal);
                if (tentative >= pathCost[neighborIndex]) {
                    continue;
                }
                pathCost[neighborIndex] = tentative;
                pathParent[neighborIndex] = currentIndex;
                const float deltaX = goalX - static_cast<float>(neighborX);
                const float deltaY = goalY - static_cast<float>(neighborY);
                const float heuristic = std::sqrt(deltaX * deltaX + deltaY * deltaY) * ROAD_BASE_COST;
                openSet.push({tentative + heuristic, neighborIndex});
            }
        }
    }
    if (pathParent[goalIndex] == -1 && goalIndex != startIndex) {
        return;
    }
    int32_t traceIndex = goalIndex;
    while (traceIndex != -1) {
        const int32_t tileX = traceIndex % worldWidth;
        const int32_t tileY = traceIndex / worldWidth;
        const WorldCoord coord{tileX, tileY};
        if (chunkStore.getTerrainAt(coord) != TerrainId::City) {
            chunkStore.setTerrainAt(coord, TerrainId::Road);
            chunkStore.setRegionAt(coord, RegionId::Settlement);
        }
        traceIndex = pathParent[traceIndex];
    }
}

void WorldGenerator::passRoads(ChunkStore& chunkStore) {
    const int32_t cityCount = static_cast<int32_t>(cities.size());
    for (int32_t fromIndex = 0; fromIndex < cityCount; ++fromIndex) {
        bool chosen[CITY_MAX_COUNT] = {false};
        for (int32_t connection = 0; connection < ROAD_CONNECTIONS_PER_CITY; ++connection) {
            int32_t bestTarget = -1;
            int64_t bestDistance = std::numeric_limits<int64_t>::max();
            for (int32_t toIndex = 0; toIndex < cityCount; ++toIndex) {
                if (toIndex == fromIndex || chosen[toIndex]) {
                    continue;
                }
                const int64_t deltaX = cities[toIndex].x - cities[fromIndex].x;
                const int64_t deltaY = cities[toIndex].y - cities[fromIndex].y;
                const int64_t distance = deltaX * deltaX + deltaY * deltaY;
                if (distance < bestDistance) {
                    bestDistance = distance;
                    bestTarget = toIndex;
                }
            }
            if (bestTarget < 0) {
                break;
            }
            chosen[bestTarget] = true;
            if (bestTarget > fromIndex) {
                carveRoad(chunkStore, cities[fromIndex], cities[bestTarget]);
            }
        }
    }
}

} // namespace Core
