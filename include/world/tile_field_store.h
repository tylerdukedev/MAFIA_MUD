#pragma once

#include "core/types.h"
#include "world/world_config.h"
#include <cstdint>
#include <vector>

namespace Core {

class ChunkStore;
class DistrictStore;

constexpr int32_t TILE_FIELD_COUNT = WorldConfig::WORLD_WIDTH_TILES * WorldConfig::WORLD_HEIGHT_TILES;

class TileFieldStore {
public:
    TileFieldStore();
    void reset();
    void seedFromDistrictStore(const DistrictStore& districtStore, const ChunkStore& chunkStore);
    float getInfluenceAt(const WorldCoord& worldCoord) const;
    float getHeatAt(const WorldCoord& worldCoord) const;
    void setInfluenceAt(const WorldCoord& worldCoord, float influence);
    void setHeatAt(const WorldCoord& worldCoord, float heat);
    float getMeanInfluence() const;
    float getMeanHeat() const;
    bool exportFields(float* outInfluence, float* outHeat, int32_t tileCount) const;
    bool importFields(const float* influenceValues, const float* heatValues, int32_t tileCount);

private:
    std::vector<float> influence;
    std::vector<float> heat;
    int32_t worldCoordToIndex(const WorldCoord& worldCoord) const;
    static float clampUnit(float value);
};

} // namespace Core
