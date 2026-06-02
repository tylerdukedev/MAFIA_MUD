#pragma once

#include "core/types.h"
#include "sim/isim_system.h"
#include "sim/sim_context.h"
#include <cstdint>
#include <vector>

namespace Core {

class WorldConfig;
class ChunkStore;
class TileFieldStore;
class DistrictStore;

class InfluenceSystem final : public ISimSystem {
public:
    void bindContext(const SimContext& simContext);
    const char* getName() const override;
    void onTick(uint64_t tickCount) override;

private:
    const WorldConfig* worldConfig = nullptr;
    const ChunkStore* chunkStore = nullptr;
    TileFieldStore* tileFieldStore = nullptr;
    DistrictStore* districtStore = nullptr;
    std::vector<float> influenceScratch;
    std::vector<float> heatScratch;
    void diffuseTileFields();
    void rollupDistrictFields();
    void applyLandmarkBias();
    float sampleNeighborAverage(const std::vector<float>& fieldValues, int32_t tileX, int32_t tileY) const;
    float getDiffusionWeight(const WorldCoord& worldCoord) const;
};

} // namespace Core
