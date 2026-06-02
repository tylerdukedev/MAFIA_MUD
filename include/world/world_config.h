#pragma once

#include "core/types.h"

namespace Core {

struct WorldConfig {
    static constexpr int32_t CHUNK_SIZE = 32;
    static constexpr int32_t WORLD_WIDTH_TILES = 512;
    static constexpr int32_t WORLD_HEIGHT_TILES = 512;
    static constexpr int32_t CHUNK_COUNT_X = WORLD_WIDTH_TILES / CHUNK_SIZE;
    static constexpr int32_t CHUNK_COUNT_Y = WORLD_HEIGHT_TILES / CHUNK_SIZE;
    static constexpr int32_t TILES_PER_CHUNK = CHUNK_SIZE * CHUNK_SIZE;
    static constexpr double DEFAULT_TICK_RATE_HZ = 20.0;
    bool isWithinWorldBounds(const WorldCoord& coord) const;
    bool isWithinChunkBounds(const ChunkCoord& coord) const;
    ChunkCoord worldToChunkCoord(const WorldCoord& worldCoord) const;
    LocalTileCoord worldToLocalTileCoord(const WorldCoord& worldCoord) const;
    WorldCoord chunkAndLocalToWorld(const ChunkCoord& chunkCoord, const LocalTileCoord& localCoord) const;
    int32_t chunkCoordToIndex(const ChunkCoord& chunkCoord) const;
    ChunkCoord chunkIndexToCoord(int32_t chunkIndex) const;
    int32_t localTileToIndex(const LocalTileCoord& localCoord) const;
};

} // namespace Core
