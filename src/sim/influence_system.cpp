#include "sim/influence_system.h"
#include "world/district_grid.h"
#include "world/landmark_table.h"
#include <algorithm>
#include <cmath>

namespace Core {

namespace {
constexpr float INFLUENCE_DIFFUSION_RATE = 0.12f;
constexpr float HEAT_DIFFUSION_RATE = 0.10f;
constexpr float INFLUENCE_DECAY = 0.985f;
constexpr float HEAT_DECAY = 0.992f;
constexpr float ROAD_DIFFUSION_BONUS = 0.08f;
constexpr float PLAZA_DIFFUSION_BONUS = 0.05f;
constexpr float LANDMARK_HEAT_PUSH = 0.015f;
constexpr float LANDMARK_INFLUENCE_PUSH = 0.004f;
} // namespace

void InfluenceSystem::bindContext(const SimContext& simContext) {
    worldConfig = simContext.worldConfig;
    chunkStore = simContext.chunkStore;
    tileFieldStore = simContext.tileFieldStore;
    districtStore = simContext.districtStore;
    influenceScratch.assign(static_cast<size_t>(TILE_FIELD_COUNT), 0.0f);
    heatScratch.assign(static_cast<size_t>(TILE_FIELD_COUNT), 0.0f);
}

const char* InfluenceSystem::getName() const {
    return "InfluenceSystem";
}

float InfluenceSystem::getDiffusionWeight(const WorldCoord& worldCoord) const {
    if (chunkStore == nullptr) {
        return 1.0f;
    }
    const TerrainId terrainId = chunkStore->getTerrainAt(worldCoord);
    if (terrainId == TerrainId::Road) {
        return 1.0f + ROAD_DIFFUSION_BONUS;
    }
    if (terrainId == TerrainId::Plaza) {
        return 1.0f + PLAZA_DIFFUSION_BONUS;
    }
    if (terrainId == TerrainId::Water) {
        return 0.35f;
    }
    return 1.0f;
}

float InfluenceSystem::sampleNeighborAverage(const std::vector<float>& fieldValues, int32_t tileX, int32_t tileY) const {
    float neighborSum = 0.0f;
    int32_t neighborCount = 0;
    const int32_t neighborOffsets[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    for (int32_t offsetIndex = 0; offsetIndex < 4; ++offsetIndex) {
        const int32_t neighborX = tileX + neighborOffsets[offsetIndex][0];
        const int32_t neighborY = tileY + neighborOffsets[offsetIndex][1];
        if (neighborX < 0 || neighborY < 0 || neighborX >= WorldConfig::WORLD_WIDTH_TILES || neighborY >= WorldConfig::WORLD_HEIGHT_TILES) {
            continue;
        }
        const int32_t neighborIndex = neighborY * WorldConfig::WORLD_WIDTH_TILES + neighborX;
        neighborSum += fieldValues[static_cast<size_t>(neighborIndex)];
        neighborCount += 1;
    }
    if (neighborCount == 0) {
        return 0.0f;
    }
    return neighborSum / static_cast<float>(neighborCount);
}

void InfluenceSystem::diffuseTileFields() {
    if (tileFieldStore == nullptr) {
        return;
    }
    for (int32_t tileY = 0; tileY < WorldConfig::WORLD_HEIGHT_TILES; ++tileY) {
        for (int32_t tileX = 0; tileX < WorldConfig::WORLD_WIDTH_TILES; ++tileX) {
            const WorldCoord worldCoord{tileX, tileY};
            const int32_t tileIndex = tileY * WorldConfig::WORLD_WIDTH_TILES + tileX;
            const float diffusionWeight = getDiffusionWeight(worldCoord);
            const float neighborInfluence = sampleNeighborAverage(influenceScratch, tileX, tileY);
            const float neighborHeat = sampleNeighborAverage(heatScratch, tileX, tileY);
            const float currentInfluence = influenceScratch[static_cast<size_t>(tileIndex)];
            const float currentHeat = heatScratch[static_cast<size_t>(tileIndex)];
            const float blendedInfluence = currentInfluence * INFLUENCE_DECAY
                + neighborInfluence * INFLUENCE_DIFFUSION_RATE * diffusionWeight;
            const float blendedHeat = currentHeat * HEAT_DECAY + neighborHeat * HEAT_DIFFUSION_RATE * diffusionWeight;
            tileFieldStore->setInfluenceAt(worldCoord, blendedInfluence);
            tileFieldStore->setHeatAt(worldCoord, blendedHeat);
        }
    }
}

void InfluenceSystem::rollupDistrictFields() {
    if (tileFieldStore == nullptr || districtStore == nullptr) {
        return;
    }
    for (int32_t districtY = 0; districtY < DISTRICT_COUNT; ++districtY) {
        for (int32_t districtX = 0; districtX < DISTRICT_COUNT; ++districtX) {
            const DistrictCoord districtCoord{districtX, districtY};
            const DistrictId districtId = DistrictGrid::districtCoordToId(districtCoord);
            DistrictRecord& district = districtStore->getDistrictMutable(districtId);
            const WorldCoord origin = DistrictGrid::districtToWorldOrigin(districtCoord);
            float influenceSum = 0.0f;
            float heatSum = 0.0f;
            int32_t sampleCount = 0;
            for (int32_t localY = 0; localY < DISTRICT_TILE_SIZE; ++localY) {
                for (int32_t localX = 0; localX < DISTRICT_TILE_SIZE; ++localX) {
                    const WorldCoord tileCoord{origin.x + localX, origin.y + localY};
                    influenceSum += tileFieldStore->getInfluenceAt(tileCoord);
                    heatSum += tileFieldStore->getHeatAt(tileCoord);
                    sampleCount += 1;
                }
            }
            if (sampleCount > 0) {
                const float inverseCount = 1.0f / static_cast<float>(sampleCount);
                district.playerInfluence = std::max(0.0f, std::min(1.0f, influenceSum * inverseCount));
                district.heat = std::max(0.0f, std::min(1.0f, heatSum * inverseCount));
            }
        }
    }
}

void InfluenceSystem::applyLandmarkBias() {
    if (districtStore == nullptr) {
        return;
    }
    for (DistrictId districtId = 0; districtId < DISTRICT_CELL_COUNT; ++districtId) {
        DistrictRecord& district = districtStore->getDistrictMutable(districtId);
        if (district.landmarkIndex < 0) {
            continue;
        }
        district.heat = std::min(1.0f, district.heat + LANDMARK_HEAT_PUSH);
        district.playerInfluence = std::min(1.0f, district.playerInfluence + LANDMARK_INFLUENCE_PUSH);
    }
}

void InfluenceSystem::onTick(uint64_t tickCount) {
    lastTickCount = tickCount;
    if (tileFieldStore == nullptr || influenceScratch.size() != static_cast<size_t>(TILE_FIELD_COUNT)) {
        return;
    }
    if (!tileFieldStore->exportFields(influenceScratch.data(), heatScratch.data(), TILE_FIELD_COUNT)) {
        return;
    }
    diffuseTileFields();
    rollupDistrictFields();
    applyLandmarkBias();
}

} // namespace Core
