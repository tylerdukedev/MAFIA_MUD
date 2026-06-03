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
    std::vector<uint8_t> economicWeight;
    std::vector<uint16_t> population;
    std::vector<uint8_t> crimePressure;
    std::vector<uint8_t> lawPressure;
    std::vector<uint8_t> businessVitality;
    std::vector<uint8_t> playerInfluence;
    std::vector<uint8_t> oppositionInfluence;
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
    const WorldConfig& getConfig() const;
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
    uint8_t getEconomicWeightAt(const WorldCoord& worldCoord) const;
    uint16_t getPopulationAt(const WorldCoord& worldCoord) const;
    uint8_t getCrimePressureAt(const WorldCoord& worldCoord) const;
    uint8_t getLawPressureAt(const WorldCoord& worldCoord) const;
    uint8_t getBusinessVitalityAt(const WorldCoord& worldCoord) const;
    uint8_t getPlayerInfluenceAt(const WorldCoord& worldCoord) const;
    uint8_t getOppositionInfluenceAt(const WorldCoord& worldCoord) const;
    void setRegionAt(const WorldCoord& worldCoord, RegionId regionId);
    void setTerrainAt(const WorldCoord& worldCoord, TerrainId terrainId);
    void setElevationAt(const WorldCoord& worldCoord, int16_t elevation);
    void setEconomicWeightAt(const WorldCoord& worldCoord, uint8_t economicWeight);
    void setPopulationAt(const WorldCoord& worldCoord, uint16_t population);
    void setCrimePressureAt(const WorldCoord& worldCoord, uint8_t crimePressure);
    void setLawPressureAt(const WorldCoord& worldCoord, uint8_t lawPressure);
    void setBusinessVitalityAt(const WorldCoord& worldCoord, uint8_t businessVitality);
    void setPlayerInfluenceAt(const WorldCoord& worldCoord, uint8_t playerInfluence);
    void setOppositionInfluenceAt(const WorldCoord& worldCoord, uint8_t oppositionInfluence);
    int32_t getTileIndexInChunk(const WorldCoord& worldCoord) const;
    void resetAll();
    bool exportFullWorldTiles(
        uint8_t* outRegionIds,
        uint8_t* outTerrainIds,
        int16_t* outElevations,
        uint32_t* outFlags,
        int32_t tileCount) const;
    bool importFullWorldTiles(
        const uint8_t* regionIds,
        const uint8_t* terrainIds,
        const int16_t* elevations,
        const uint32_t* flags,
        int32_t tileCount);
    bool exportFullWorldVitality(
        uint8_t* outEconomicWeights,
        uint16_t* outPopulations,
        uint8_t* outCrimePressures,
        uint8_t* outLawPressures,
        uint8_t* outBusinessVitalities,
        uint8_t* outPlayerInfluences,
        uint8_t* outOppositionInfluences,
        int32_t tileCount) const;
    bool importFullWorldVitality(
        const uint8_t* economicWeights,
        const uint16_t* populations,
        const uint8_t* crimePressures,
        const uint8_t* lawPressures,
        const uint8_t* businessVitalities,
        const uint8_t* playerInfluences,
        const uint8_t* oppositionInfluences,
        int32_t tileCount);

private:
    const WorldConfig& config;
    std::vector<Chunk> chunks;
    int32_t activeChunkCount = 0;
    ChunkId allocateChunk(const ChunkCoord& chunkCoord);
};

} // namespace Core
