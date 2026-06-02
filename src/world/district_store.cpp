#include "world/district_store.h"
#include "world/chunk_store.h"
#include "world/landmark_table.h"
#include <algorithm>

namespace Core {

DistrictStore::DistrictStore() {
    reset();
}

void DistrictStore::reset() {
    districts.assign(static_cast<size_t>(DISTRICT_CELL_COUNT), DistrictRecord{});
    regionVotes.assign(static_cast<size_t>(DISTRICT_CELL_COUNT), RegionVote{});
}

void DistrictStore::accumulateTileIntoDistrict(DistrictId districtId, RegionId regionId, TerrainId terrainId) {
    if (districtId >= DISTRICT_CELL_COUNT) {
        return;
    }
    DistrictRecord& district = districts[static_cast<size_t>(districtId)];
    const int32_t regionIndex = static_cast<int32_t>(regionId);
    if (regionIndex >= 0 && regionIndex < static_cast<int32_t>(RegionId::COUNT)) {
        regionVotes[static_cast<size_t>(districtId)].counts[regionIndex] += 1;
    }
    if (terrainId == TerrainId::Water) {
        district.waterTileCount += 1;
    } else if (terrainId != TerrainId::None) {
        district.landTileCount += 1;
    }
}

void DistrictStore::finalizeDominantRegions() {
    for (DistrictId districtId = 0; districtId < DISTRICT_CELL_COUNT; ++districtId) {
        const RegionVote& vote = regionVotes[static_cast<size_t>(districtId)];
        int32_t bestRegionIndex = 0;
        int32_t bestVoteCount = 0;
        for (int32_t regionIndex = 1; regionIndex < static_cast<int32_t>(RegionId::COUNT); ++regionIndex) {
            if (vote.counts[regionIndex] > bestVoteCount) {
                bestVoteCount = vote.counts[regionIndex];
                bestRegionIndex = regionIndex;
            }
        }
        districts[static_cast<size_t>(districtId)].dominantRegionId = static_cast<RegionId>(bestRegionIndex);
    }
}

void DistrictStore::buildFromChunkStore(const ChunkStore& chunkStore) {
    reset();
    for (int32_t districtY = 0; districtY < DISTRICT_COUNT; ++districtY) {
        for (int32_t districtX = 0; districtX < DISTRICT_COUNT; ++districtX) {
            const DistrictCoord districtCoord{districtX, districtY};
            const DistrictId districtId = DistrictGrid::districtCoordToId(districtCoord);
            const WorldCoord origin = DistrictGrid::districtToWorldOrigin(districtCoord);
            for (int32_t localY = 0; localY < DISTRICT_TILE_SIZE; ++localY) {
                for (int32_t localX = 0; localX < DISTRICT_TILE_SIZE; ++localX) {
                    const WorldCoord tileCoord{origin.x + localX, origin.y + localY};
                    if (!chunkStore.hasTileAt(tileCoord)) {
                        continue;
                    }
                    accumulateTileIntoDistrict(
                        districtId,
                        chunkStore.getRegionAt(tileCoord),
                        chunkStore.getTerrainAt(tileCoord));
                }
            }
        }
    }
    finalizeDominantRegions();
    for (int32_t landmarkIndex = 0; landmarkIndex < getLandmarkCount(); ++landmarkIndex) {
        const LandmarkDefinition* landmark = getLandmarkDefinition(landmarkIndex);
        if (landmark == nullptr) {
            continue;
        }
        const DistrictId districtId = findDistrictForWorldCoord(WorldCoord{landmark->tileX, landmark->tileY});
        if (districtId >= DISTRICT_CELL_COUNT) {
            continue;
        }
        DistrictRecord& district = districts[static_cast<size_t>(districtId)];
        district.landmarkIndex = static_cast<int16_t>(landmarkIndex);
        district.heat = LANDMARK_BASE_HEAT;
        district.stability = 1.0f - LANDMARK_CONTROL_DIFFICULTY;
    }
}

int32_t DistrictStore::getDistrictCount() const {
    return DISTRICT_CELL_COUNT;
}

const DistrictRecord& DistrictStore::getDistrict(DistrictId districtId) const {
    static const DistrictRecord emptyRecord{};
    if (districtId >= DISTRICT_CELL_COUNT) {
        return emptyRecord;
    }
    return districts[static_cast<size_t>(districtId)];
}

DistrictRecord& DistrictStore::getDistrictMutable(DistrictId districtId) {
    static DistrictRecord emptyRecord{};
    if (districtId >= DISTRICT_CELL_COUNT) {
        return emptyRecord;
    }
    return districts[static_cast<size_t>(districtId)];
}

DistrictId DistrictStore::findDistrictForLandmark(int32_t landmarkIndex) const {
    const LandmarkDefinition* landmark = getLandmarkDefinition(landmarkIndex);
    if (landmark == nullptr) {
        return INVALID_DISTRICT_ID;
    }
    return findDistrictForWorldCoord(WorldCoord{landmark->tileX, landmark->tileY});
}

DistrictId DistrictStore::findDistrictForWorldCoord(const WorldCoord& worldCoord) const {
    const DistrictCoord districtCoord = DistrictGrid::worldToDistrictCoord(worldCoord);
    return DistrictGrid::districtCoordToId(districtCoord);
}

float DistrictStore::getMeanHeat() const {
    float totalHeat = 0.0f;
    for (const DistrictRecord& district : districts) {
        totalHeat += district.heat;
    }
    return totalHeat / static_cast<float>(DISTRICT_CELL_COUNT);
}

float DistrictStore::getMeanPlayerInfluence() const {
    float totalInfluence = 0.0f;
    for (const DistrictRecord& district : districts) {
        totalInfluence += district.playerInfluence;
    }
    return totalInfluence / static_cast<float>(DISTRICT_CELL_COUNT);
}

bool DistrictStore::exportSimFields(
    float* outHeat,
    float* outStability,
    float* outPlayerInfluence,
    int32_t districtCount) const {
    if (districtCount != DISTRICT_CELL_COUNT || outHeat == nullptr || outStability == nullptr || outPlayerInfluence == nullptr) {
        return false;
    }
    for (DistrictId districtId = 0; districtId < DISTRICT_CELL_COUNT; ++districtId) {
        const DistrictRecord& district = districts[static_cast<size_t>(districtId)];
        outHeat[districtId] = district.heat;
        outStability[districtId] = district.stability;
        outPlayerInfluence[districtId] = district.playerInfluence;
    }
    return true;
}

bool DistrictStore::importSimFields(
    const float* heatValues,
    const float* stabilityValues,
    const float* playerInfluenceValues,
    int32_t districtCount) {
    if (districtCount != DISTRICT_CELL_COUNT || heatValues == nullptr || stabilityValues == nullptr || playerInfluenceValues == nullptr) {
        return false;
    }
    for (DistrictId districtId = 0; districtId < DISTRICT_CELL_COUNT; ++districtId) {
        DistrictRecord& district = districts[static_cast<size_t>(districtId)];
        district.heat = heatValues[districtId];
        district.stability = stabilityValues[districtId];
        district.playerInfluence = playerInfluenceValues[districtId];
    }
    return true;
}

} // namespace Core
