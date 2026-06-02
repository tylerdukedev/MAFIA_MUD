#pragma once

#include "core/types.h"
#include "world/district_grid.h"
#include <cstdint>
#include <vector>

namespace Core {

class ChunkStore;

struct DistrictRecord {
    RegionId dominantRegionId = RegionId::None;
    int16_t landmarkIndex = -1;
    int16_t landTileCount = 0;
    int16_t waterTileCount = 0;
    float heat = 0.0f;
    float stability = 0.0f;
    float playerInfluence = 0.0f;
};

class DistrictStore {
public:
    DistrictStore();
    void reset();
    void buildFromChunkStore(const ChunkStore& chunkStore);
    int32_t getDistrictCount() const;
    const DistrictRecord& getDistrict(DistrictId districtId) const;
    DistrictRecord& getDistrictMutable(DistrictId districtId);
    DistrictId findDistrictForLandmark(int32_t landmarkIndex) const;
    DistrictId findDistrictForWorldCoord(const WorldCoord& worldCoord) const;
    float getMeanHeat() const;
    float getMeanPlayerInfluence() const;
    bool exportSimFields(
        float* outHeat,
        float* outStability,
        float* outPlayerInfluence,
        int32_t districtCount) const;
    bool importSimFields(
        const float* heatValues,
        const float* stabilityValues,
        const float* playerInfluenceValues,
        int32_t districtCount);

private:
    std::vector<DistrictRecord> districts;
    void accumulateTileIntoDistrict(DistrictId districtId, RegionId regionId, TerrainId terrainId);
    void finalizeDominantRegions();
    struct RegionVote {
        int32_t counts[static_cast<int32_t>(RegionId::COUNT)] = {};
    };
    std::vector<RegionVote> regionVotes;
};

} // namespace Core
