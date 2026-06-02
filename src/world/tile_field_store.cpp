#include "world/tile_field_store.h"
#include "world/chunk_store.h"
#include "world/district_grid.h"
#include "world/district_store.h"
#include <algorithm>
#include <cmath>

namespace Core {

TileFieldStore::TileFieldStore() {
    reset();
}

void TileFieldStore::reset() {
    influence.assign(static_cast<size_t>(TILE_FIELD_COUNT), 0.0f);
    heat.assign(static_cast<size_t>(TILE_FIELD_COUNT), 0.0f);
}

int32_t TileFieldStore::worldCoordToIndex(const WorldCoord& worldCoord) const {
    if (worldCoord.x < 0 || worldCoord.y < 0
        || worldCoord.x >= WorldConfig::WORLD_WIDTH_TILES
        || worldCoord.y >= WorldConfig::WORLD_HEIGHT_TILES) {
        return -1;
    }
    return worldCoord.y * WorldConfig::WORLD_WIDTH_TILES + worldCoord.x;
}

float TileFieldStore::clampUnit(float value) {
    return std::max(0.0f, std::min(1.0f, value));
}

void TileFieldStore::seedFromDistrictStore(const DistrictStore& districtStore, const ChunkStore& chunkStore) {
    reset();
    for (int32_t districtY = 0; districtY < DISTRICT_COUNT; ++districtY) {
        for (int32_t districtX = 0; districtX < DISTRICT_COUNT; ++districtX) {
            const DistrictCoord districtCoord{districtX, districtY};
            const DistrictId districtId = DistrictGrid::districtCoordToId(districtCoord);
            const DistrictRecord& district = districtStore.getDistrict(districtId);
            if (district.heat <= 0.0f && district.playerInfluence <= 0.0f) {
                continue;
            }
            const WorldCoord origin = DistrictGrid::districtToWorldOrigin(districtCoord);
            for (int32_t localY = 0; localY < DISTRICT_TILE_SIZE; ++localY) {
                for (int32_t localX = 0; localX < DISTRICT_TILE_SIZE; ++localX) {
                    const WorldCoord tileCoord{origin.x + localX, origin.y + localY};
                    if (!chunkStore.hasTileAt(tileCoord)) {
                        continue;
                    }
                    setHeatAt(tileCoord, district.heat * 0.25f);
                    setInfluenceAt(tileCoord, district.playerInfluence);
                }
            }
        }
    }
}

float TileFieldStore::getInfluenceAt(const WorldCoord& worldCoord) const {
    const int32_t tileIndex = worldCoordToIndex(worldCoord);
    if (tileIndex < 0) {
        return 0.0f;
    }
    return influence[static_cast<size_t>(tileIndex)];
}

float TileFieldStore::getHeatAt(const WorldCoord& worldCoord) const {
    const int32_t tileIndex = worldCoordToIndex(worldCoord);
    if (tileIndex < 0) {
        return 0.0f;
    }
    return heat[static_cast<size_t>(tileIndex)];
}

void TileFieldStore::setInfluenceAt(const WorldCoord& worldCoord, float influenceValue) {
    const int32_t tileIndex = worldCoordToIndex(worldCoord);
    if (tileIndex < 0) {
        return;
    }
    influence[static_cast<size_t>(tileIndex)] = clampUnit(influenceValue);
}

void TileFieldStore::setHeatAt(const WorldCoord& worldCoord, float heatValue) {
    const int32_t tileIndex = worldCoordToIndex(worldCoord);
    if (tileIndex < 0) {
        return;
    }
    heat[static_cast<size_t>(tileIndex)] = clampUnit(heatValue);
}

float TileFieldStore::getMeanInfluence() const {
    float totalInfluence = 0.0f;
    for (float value : influence) {
        totalInfluence += value;
    }
    return totalInfluence / static_cast<float>(TILE_FIELD_COUNT);
}

float TileFieldStore::getMeanHeat() const {
    float totalHeat = 0.0f;
    for (float value : heat) {
        totalHeat += value;
    }
    return totalHeat / static_cast<float>(TILE_FIELD_COUNT);
}

bool TileFieldStore::exportFields(float* outInfluence, float* outHeat, int32_t tileCount) const {
    if (tileCount != TILE_FIELD_COUNT || outInfluence == nullptr || outHeat == nullptr) {
        return false;
    }
    for (int32_t tileIndex = 0; tileIndex < TILE_FIELD_COUNT; ++tileIndex) {
        outInfluence[tileIndex] = influence[static_cast<size_t>(tileIndex)];
        outHeat[tileIndex] = heat[static_cast<size_t>(tileIndex)];
    }
    return true;
}

bool TileFieldStore::importFields(const float* influenceValues, const float* heatValues, int32_t tileCount) {
    if (tileCount != TILE_FIELD_COUNT || influenceValues == nullptr || heatValues == nullptr) {
        return false;
    }
    for (int32_t tileIndex = 0; tileIndex < TILE_FIELD_COUNT; ++tileIndex) {
        influence[static_cast<size_t>(tileIndex)] = clampUnit(influenceValues[tileIndex]);
        heat[static_cast<size_t>(tileIndex)] = clampUnit(heatValues[tileIndex]);
    }
    return true;
}

} // namespace Core
