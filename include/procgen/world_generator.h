#pragma once

#include "world/chunk_store.h"
#include "world/world_config.h"
#include <cstdint>
#include <vector>

namespace Core {

constexpr uint64_t DEFAULT_WORLD_SEED = 0xCA5E1B1CEULL;

class WorldGenerator {
public:
    void generate(const WorldConfig& worldConfig, ChunkStore& chunkStore, uint64_t worldSeed);

private:
    uint64_t worldSeed = DEFAULT_WORLD_SEED;
    int32_t worldWidth = 0;
    int32_t worldHeight = 0;
    std::vector<uint8_t> bakedRegion;
    void decodeBakedMap();
    uint8_t sampleBakedCode(int32_t x, int32_t y) const;
    void passBoroughs(ChunkStore& chunkStore);
    void passStreets(ChunkStore& chunkStore);
    void passParks(ChunkStore& chunkStore);
    void passElevation(ChunkStore& chunkStore);
    bool isCentralParkTile(int32_t x, int32_t y) const;
};

} // namespace Core
