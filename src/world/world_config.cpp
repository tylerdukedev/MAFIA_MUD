#include "world/world_config.h"

namespace Core {

bool WorldConfig::isWithinWorldBounds(const WorldCoord& coord) const {
    if (coord.x < 0 || coord.y < 0) {
        return false;
    }
    if (coord.x >= WORLD_WIDTH_TILES || coord.y >= WORLD_HEIGHT_TILES) {
        return false;
    }
    return true;
}

bool WorldConfig::isWithinChunkBounds(const ChunkCoord& coord) const {
    if (coord.x < 0 || coord.y < 0) {
        return false;
    }
    if (coord.x >= CHUNK_COUNT_X || coord.y >= CHUNK_COUNT_Y) {
        return false;
    }
    return true;
}

ChunkCoord WorldConfig::worldToChunkCoord(const WorldCoord& worldCoord) const {
    ChunkCoord chunkCoord;
    chunkCoord.x = worldCoord.x / CHUNK_SIZE;
    chunkCoord.y = worldCoord.y / CHUNK_SIZE;
    return chunkCoord;
}

LocalTileCoord WorldConfig::worldToLocalTileCoord(const WorldCoord& worldCoord) const {
    LocalTileCoord localCoord;
    localCoord.x = static_cast<uint16_t>(worldCoord.x % CHUNK_SIZE);
    localCoord.y = static_cast<uint16_t>(worldCoord.y % CHUNK_SIZE);
    return localCoord;
}

WorldCoord WorldConfig::chunkAndLocalToWorld(const ChunkCoord& chunkCoord, const LocalTileCoord& localCoord) const {
    WorldCoord worldCoord;
    worldCoord.x = chunkCoord.x * CHUNK_SIZE + static_cast<int32_t>(localCoord.x);
    worldCoord.y = chunkCoord.y * CHUNK_SIZE + static_cast<int32_t>(localCoord.y);
    return worldCoord;
}

int32_t WorldConfig::chunkCoordToIndex(const ChunkCoord& chunkCoord) const {
    return chunkCoord.y * CHUNK_COUNT_X + chunkCoord.x;
}

ChunkCoord WorldConfig::chunkIndexToCoord(int32_t chunkIndex) const {
    ChunkCoord chunkCoord;
    chunkCoord.x = chunkIndex % CHUNK_COUNT_X;
    chunkCoord.y = chunkIndex / CHUNK_COUNT_X;
    return chunkCoord;
}

int32_t WorldConfig::localTileToIndex(const LocalTileCoord& localCoord) const {
    return static_cast<int32_t>(localCoord.y) * CHUNK_SIZE + static_cast<int32_t>(localCoord.x);
}

} // namespace Core
