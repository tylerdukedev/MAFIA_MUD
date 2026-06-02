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

void ChunkStore::resetAll() {
    for (Chunk& chunk : chunks) {
        chunk.id = INVALID_CHUNK_ID;
        chunk.coord = ChunkCoord{};
        chunk.isActive = false;
        chunk.tiles.regionId.clear();
        chunk.tiles.terrainId.clear();
        chunk.tiles.elevation.clear();
        chunk.tiles.flags.clear();
    }
    activeChunkCount = 0;
}

bool ChunkStore::exportFullWorldTiles(
    uint8_t* outRegionIds,
    uint8_t* outTerrainIds,
    int16_t* outElevations,
    uint32_t* outFlags,
    int32_t tileCount) const {
    const int32_t expectedTileCount = config.WORLD_WIDTH_TILES * config.WORLD_HEIGHT_TILES;
    if (tileCount != expectedTileCount || outRegionIds == nullptr || outTerrainIds == nullptr || outElevations == nullptr || outFlags == nullptr) {
        return false;
    }
    int32_t writeIndex = 0;
    for (int32_t tileY = 0; tileY < config.WORLD_HEIGHT_TILES; ++tileY) {
        for (int32_t tileX = 0; tileX < config.WORLD_WIDTH_TILES; ++tileX) {
            const WorldCoord coord{tileX, tileY};
            outRegionIds[writeIndex] = static_cast<uint8_t>(getRegionAt(coord));
            outTerrainIds[writeIndex] = static_cast<uint8_t>(getTerrainAt(coord));
            outElevations[writeIndex] = getElevationAt(coord);
            const ChunkCoord chunkCoord = config.worldToChunkCoord(coord);
            const Chunk* chunk = getChunkAtCoord(chunkCoord);
            if (chunk != nullptr) {
                const int32_t tileIndex = getTileIndexInChunk(coord);
                outFlags[writeIndex] = chunk->tiles.flags[static_cast<size_t>(tileIndex)];
            } else {
                outFlags[writeIndex] = 0U;
            }
            ++writeIndex;
        }
    }
    return true;
}

bool ChunkStore::importFullWorldTiles(
    const uint8_t* regionIds,
    const uint8_t* terrainIds,
    const int16_t* elevations,
    const uint32_t* flags,
    int32_t tileCount) {
    const int32_t expectedTileCount = config.WORLD_WIDTH_TILES * config.WORLD_HEIGHT_TILES;
    if (tileCount != expectedTileCount || regionIds == nullptr || terrainIds == nullptr || elevations == nullptr || flags == nullptr) {
        return false;
    }
    resetAll();
    int32_t readIndex = 0;
    for (int32_t tileY = 0; tileY < config.WORLD_HEIGHT_TILES; ++tileY) {
        for (int32_t tileX = 0; tileX < config.WORLD_WIDTH_TILES; ++tileX) {
            const WorldCoord coord{tileX, tileY};
            const ChunkCoord chunkCoord = config.worldToChunkCoord(coord);
            const ChunkId chunkId = getOrCreateChunk(chunkCoord);
            Chunk* chunk = getChunk(chunkId);
            if (chunk == nullptr) {
                return false;
            }
            const int32_t tileIndex = getTileIndexInChunk(coord);
            chunk->tiles.regionId[static_cast<size_t>(tileIndex)] = static_cast<RegionId>(regionIds[readIndex]);
            chunk->tiles.terrainId[static_cast<size_t>(tileIndex)] = static_cast<TerrainId>(terrainIds[readIndex]);
            chunk->tiles.elevation[static_cast<size_t>(tileIndex)] = elevations[readIndex];
            chunk->tiles.flags[static_cast<size_t>(tileIndex)] = flags[readIndex];
            ++readIndex;
        }
    }
    return true;
}

} // namespace Core
