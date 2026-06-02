#pragma once

#include "core/types.h"
#include "world/world_config.h"
#include <cstdint>
#include <vector>

namespace Core {

struct ChunkTileData {
    std::vector<RegionId> regionId;
    std::vector<TerrainId> terrainId;
    std::vector<int16_t> elevation;
    std::vector<uint32_t> flags;
    void initialize(int32_t tileCount);
    bool isInitialized() const;
};

struct Chunk {
    ChunkId id = INVALID_CHUNK_ID;
    ChunkCoord coord;
    bool isActive = false;
    ChunkTileData tiles;
};

class ChunkStore {
public:
    explicit ChunkStore(const WorldConfig& config);
    int32_t getTotalChunkCount() const;
    int32_t getActiveChunkCount() const;
    ChunkId getOrCreateChunk(const ChunkCoord& chunkCoord);
    const Chunk* getChunk(ChunkId chunkId) const;
    Chunk* getChunk(ChunkId chunkId);
    const Chunk* getChunkAtCoord(const ChunkCoord& chunkCoord) const;
    bool hasTileAt(const WorldCoord& worldCoord) const;
    RegionId getRegionAt(const WorldCoord& worldCoord) const;
    TerrainId getTerrainAt(const WorldCoord& worldCoord) const;
    int16_t getElevationAt(const WorldCoord& worldCoord) const;
    void setRegionAt(const WorldCoord& worldCoord, RegionId regionId);
    void setTerrainAt(const WorldCoord& worldCoord, TerrainId terrainId);
    void setElevationAt(const WorldCoord& worldCoord, int16_t elevation);
    int32_t getTileIndexInChunk(const WorldCoord& worldCoord) const;

private:
    const WorldConfig& config;
    std::vector<Chunk> chunks;
    int32_t activeChunkCount = 0;
    ChunkId allocateChunk(const ChunkCoord& chunkCoord);
};

} // namespace Core
