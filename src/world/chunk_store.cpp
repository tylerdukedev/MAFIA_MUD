#include "world/chunk_store.h"

namespace Core {

void ChunkTileData::initialize(int32_t tileCount) {
    regionId.assign(static_cast<size_t>(tileCount), RegionId::None);
    terrainId.assign(static_cast<size_t>(tileCount), TerrainId::None);
    elevation.assign(static_cast<size_t>(tileCount), 0);
    flags.assign(static_cast<size_t>(tileCount), 0U);
}

bool ChunkTileData::isInitialized() const {
    return !regionId.empty();
}

ChunkStore::ChunkStore(const WorldConfig& worldConfig)
    : config(worldConfig)
    , chunks(static_cast<size_t>(worldConfig.CHUNK_COUNT_X * worldConfig.CHUNK_COUNT_Y)) {
    chunks.shrink_to_fit();
}

int32_t ChunkStore::getTotalChunkCount() const {
    return static_cast<int32_t>(chunks.size());
}

int32_t ChunkStore::getActiveChunkCount() const {
    return activeChunkCount;
}

ChunkId ChunkStore::getOrCreateChunk(const ChunkCoord& chunkCoord) {
    if (!config.isWithinChunkBounds(chunkCoord)) {
        return INVALID_CHUNK_ID;
    }
    const int32_t chunkIndex = config.chunkCoordToIndex(chunkCoord);
    Chunk& chunk = chunks[static_cast<size_t>(chunkIndex)];
    if (chunk.isActive) {
        return chunk.id;
    }
    return allocateChunk(chunkCoord);
}

const Chunk* ChunkStore::getChunk(ChunkId chunkId) const {
    if (chunkId >= chunks.size()) {
        return nullptr;
    }
    const Chunk& chunk = chunks[chunkId];
    if (!chunk.isActive) {
        return nullptr;
    }
    return &chunk;
}

Chunk* ChunkStore::getChunk(ChunkId chunkId) {
    if (chunkId >= chunks.size()) {
        return nullptr;
    }
    Chunk& chunk = chunks[chunkId];
    if (!chunk.isActive) {
        return nullptr;
    }
    return &chunk;
}

const Chunk* ChunkStore::getChunkAtCoord(const ChunkCoord& chunkCoord) const {
    if (!config.isWithinChunkBounds(chunkCoord)) {
        return nullptr;
    }
    const int32_t chunkIndex = config.chunkCoordToIndex(chunkCoord);
    const Chunk& chunk = chunks[static_cast<size_t>(chunkIndex)];
    if (!chunk.isActive) {
        return nullptr;
    }
    return &chunk;
}

bool ChunkStore::hasTileAt(const WorldCoord& worldCoord) const {
    if (!config.isWithinWorldBounds(worldCoord)) {
        return false;
    }
    const ChunkCoord chunkCoord = config.worldToChunkCoord(worldCoord);
    return getChunkAtCoord(chunkCoord) != nullptr;
}

int32_t ChunkStore::getTileIndexInChunk(const WorldCoord& worldCoord) const {
    const LocalTileCoord localCoord = config.worldToLocalTileCoord(worldCoord);
    return config.localTileToIndex(localCoord);
}

RegionId ChunkStore::getRegionAt(const WorldCoord& worldCoord) const {
    if (!config.isWithinWorldBounds(worldCoord)) {
        return RegionId::None;
    }
    const ChunkCoord chunkCoord = config.worldToChunkCoord(worldCoord);
    const Chunk* chunk = getChunkAtCoord(chunkCoord);
    if (chunk == nullptr) {
        return RegionId::None;
    }
    const int32_t tileIndex = getTileIndexInChunk(worldCoord);
    return chunk->tiles.regionId[static_cast<size_t>(tileIndex)];
}

TerrainId ChunkStore::getTerrainAt(const WorldCoord& worldCoord) const {
    if (!config.isWithinWorldBounds(worldCoord)) {
        return TerrainId::None;
    }
    const ChunkCoord chunkCoord = config.worldToChunkCoord(worldCoord);
    const Chunk* chunk = getChunkAtCoord(chunkCoord);
    if (chunk == nullptr) {
        return TerrainId::None;
    }
    const int32_t tileIndex = getTileIndexInChunk(worldCoord);
    return chunk->tiles.terrainId[static_cast<size_t>(tileIndex)];
}

void ChunkStore::setRegionAt(const WorldCoord& worldCoord, RegionId regionId) {
    if (!config.isWithinWorldBounds(worldCoord)) {
        return;
    }
    const ChunkCoord chunkCoord = config.worldToChunkCoord(worldCoord);
    const ChunkId chunkId = getOrCreateChunk(chunkCoord);
    Chunk* chunk = getChunk(chunkId);
    if (chunk == nullptr) {
        return;
    }
    const int32_t tileIndex = getTileIndexInChunk(worldCoord);
    chunk->tiles.regionId[static_cast<size_t>(tileIndex)] = regionId;
}


int16_t ChunkStore::getElevationAt(const WorldCoord& worldCoord) const {
    if (!config.isWithinWorldBounds(worldCoord)) {
        return 0;
    }
    const ChunkCoord chunkCoord = config.worldToChunkCoord(worldCoord);
    const Chunk* chunk = getChunkAtCoord(chunkCoord);
    if (chunk == nullptr) {
        return 0;
    }
    const int32_t tileIndex = getTileIndexInChunk(worldCoord);
    return chunk->tiles.elevation[static_cast<size_t>(tileIndex)];
}

void ChunkStore::setElevationAt(const WorldCoord& worldCoord, int16_t elevation) {
    if (!config.isWithinWorldBounds(worldCoord)) {
        return;
    }
    const ChunkCoord chunkCoord = config.worldToChunkCoord(worldCoord);
    const ChunkId chunkId = getOrCreateChunk(chunkCoord);
    Chunk* chunk = getChunk(chunkId);
    if (chunk == nullptr) {
        return;
    }
    const int32_t tileIndex = getTileIndexInChunk(worldCoord);
    chunk->tiles.elevation[static_cast<size_t>(tileIndex)] = elevation;
}

void ChunkStore::setTerrainAt(const WorldCoord& worldCoord, TerrainId terrainId) {
    if (!config.isWithinWorldBounds(worldCoord)) {
        return;
    }
    const ChunkCoord chunkCoord = config.worldToChunkCoord(worldCoord);
    const ChunkId chunkId = getOrCreateChunk(chunkCoord);
    Chunk* chunk = getChunk(chunkId);
    if (chunk == nullptr) {
        return;
    }
    const int32_t tileIndex = getTileIndexInChunk(worldCoord);
    chunk->tiles.terrainId[static_cast<size_t>(tileIndex)] = terrainId;
}

ChunkId ChunkStore::allocateChunk(const ChunkCoord& chunkCoord) {
    const int32_t chunkIndex = config.chunkCoordToIndex(chunkCoord);
    Chunk& chunk = chunks[static_cast<size_t>(chunkIndex)];
    chunk.id = static_cast<ChunkId>(chunkIndex);
    chunk.coord = chunkCoord;
    chunk.isActive = true;
    chunk.tiles.initialize(config.TILES_PER_CHUNK);
    ++activeChunkCount;
    return chunk.id;
}

} // namespace Core
