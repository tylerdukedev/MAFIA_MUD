#pragma once

#include "world/chunk_store.h"
#include "world/world_config.h"
#include <cstdint>
#include <vector>

namespace Core {

constexpr uint64_t DEFAULT_WORLD_SEED = 0xCA5E1B1CEULL;

struct CityRecord {
    int32_t x = 0;
    int32_t y = 0;
    int32_t radius = 0;
};

class WorldGenerator {
public:
    void generate(const WorldConfig& worldConfig, ChunkStore& chunkStore, uint64_t worldSeed);

private:
    uint64_t worldSeed = DEFAULT_WORLD_SEED;
    int32_t worldWidth = 0;
    int32_t worldHeight = 0;
    int32_t continentCenterX = 0;
    int32_t continentCenterY = 0;
    std::vector<float> heightField;
    std::vector<float> moistureField;
    std::vector<uint8_t> waterField;
    std::vector<float> pathCost;
    std::vector<int32_t> pathParent;
    std::vector<uint8_t> pathClosed;
    std::vector<CityRecord> cities;

    void allocateFields(const WorldConfig& worldConfig);
    void passElevation();
    void passMoisture();
    void passClassify(ChunkStore& chunkStore);
    void passRivers(ChunkStore& chunkStore);
    void passCities(ChunkStore& chunkStore);
    void passRoads(ChunkStore& chunkStore);
    float fractalNoise(float sampleX, float sampleY, uint64_t streamSeed, int32_t octaves) const;
    void normalizeField(std::vector<float>& field) const;
    int32_t tileIndex(int32_t x, int32_t y) const;
    bool isWaterIndex(int32_t fieldIndex) const;
    TerrainId classifyTerrain(float normalizedHeight, float moisture) const;
    RegionId regionForTerrain(TerrainId terrainId) const;
    void traceRiver(ChunkStore& chunkStore, int32_t sourceX, int32_t sourceY);
    float scoreCitySite(const ChunkStore& chunkStore, int32_t x, int32_t y) const;
    void stampCity(ChunkStore& chunkStore, const CityRecord& city);
    float computeStepCost(const ChunkStore& chunkStore, int32_t toX, int32_t toY, bool isDiagonal) const;
    void carveRoad(ChunkStore& chunkStore, const CityRecord& cityA, const CityRecord& cityB);
};

} // namespace Core
