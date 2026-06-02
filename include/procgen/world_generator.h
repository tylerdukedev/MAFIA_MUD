#pragma once

#include "world/chunk_store.h"
#include "world/world_config.h"
#include <cstdint>

namespace Core {

constexpr uint64_t DEFAULT_WORLD_SEED = 0xCA5E1B1CEULL;

struct CityLayoutParams {
    float cityCenterX = 256.0f;
    float cityCenterY = 256.0f;
    float cityRadius = 230.0f;
    int32_t verticalRiverX = 148;
    int32_t eastRiverWestX = 318;
    int32_t eastRiverEastX = 338;
    int32_t harborNorthY = 88;
    int32_t parkCenterX = 248;
    int32_t parkCenterY = 176;
    int32_t parkHalfWidth = 34;
    int32_t parkHalfHeight = 24;
};

class WorldGenerator {
public:
    void generate(const WorldConfig& worldConfig, ChunkStore& chunkStore, uint64_t worldSeed);

private:
    uint64_t worldSeed = DEFAULT_WORLD_SEED;
    CityLayoutParams layoutParams{};
    void deriveLayoutParams();
    void passGeography(const WorldConfig& worldConfig, ChunkStore& chunkStore);
    void passUrbanFootprint(const WorldConfig& worldConfig, ChunkStore& chunkStore);
    void passStreetGrid(const WorldConfig& worldConfig, ChunkStore& chunkStore);
    void passDistricts(const WorldConfig& worldConfig, ChunkStore& chunkStore);
    void passCityBlocks(const WorldConfig& worldConfig, ChunkStore& chunkStore);
    bool isWaterTile(int32_t x, int32_t y) const;
    bool isUrbanTile(int32_t x, int32_t y) const;
    bool isRoadTile(int32_t x, int32_t y) const;
    bool isCentralParkTile(int32_t x, int32_t y) const;
    bool isMajorPlazaTile(int32_t x, int32_t y) const;
    RegionId pickDistrict(int32_t x, int32_t y) const;
};

} // namespace Core
