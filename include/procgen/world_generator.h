#pragma once

#include "world/chunk_store.h"
#include "world/world_config.h"
#include <cstdint>

namespace Core {

constexpr uint64_t DEFAULT_WORLD_SEED = 0xCA5E1B1CEULL;

class WorldGenerator {
public:
    void generate(const WorldConfig& worldConfig, ChunkStore& chunkStore, uint64_t worldSeed);

private:
    uint64_t worldSeed = DEFAULT_WORLD_SEED;
    void passGeography(const WorldConfig& worldConfig, ChunkStore& chunkStore);
    void passRegions(const WorldConfig& worldConfig, ChunkStore& chunkStore);
    void passTerrain(const WorldConfig& worldConfig, ChunkStore& chunkStore);
    bool isWaterTile(float normalizedX, float normalizedY) const;
    RegionId pickRegionForLand(float normalizedX, float normalizedY) const;
};

} // namespace Core
