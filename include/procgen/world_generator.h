#pragma once

#include "world/chunk_store.h"
#include "world/world_config.h"
#include <cstdint>

namespace Core {

constexpr uint64_t DEFAULT_WORLD_SEED = 0xCA5E1B1CEULL;

struct BoroughResult {
    RegionId nearest = RegionId::None;
    RegionId second = RegionId::None;
    float boundaryGap = 1.0f;
    bool nearestIsWater = false;
};

class WorldGenerator {
public:
    void generate(const WorldConfig& worldConfig, ChunkStore& chunkStore, uint64_t worldSeed);

private:
    uint64_t worldSeed = DEFAULT_WORLD_SEED;
    int32_t worldWidth = 0;
    int32_t worldHeight = 0;
    void passBoroughs(ChunkStore& chunkStore);
    void passStreets(ChunkStore& chunkStore);
    void passParks(ChunkStore& chunkStore);
    void passElevation(ChunkStore& chunkStore);
    float warpNoise(float sampleX, float sampleY, uint64_t streamSeed) const;
    BoroughResult classifyBorough(float normalizedX, float normalizedY) const;
    bool isAtlanticTile(float normalizedX, float normalizedY) const;
    bool isWaterBoundary(RegionId nearest, RegionId second) const;
    bool isMainlandRegion(RegionId regionId) const;
    bool isCentralParkTile(int32_t x, int32_t y) const;
};

} // namespace Core
