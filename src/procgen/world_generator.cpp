#include "procgen/world_generator.h"
#include "procgen/borough_map_data.h"
#include "utils/seed_hash.h"
#include <cstdint>
#include <vector>

namespace Core {

namespace {
constexpr uint8_t CODE_WATER = 0;
constexpr uint8_t CODE_QUEENS = 3;
constexpr uint8_t CODE_NEW_JERSEY = 6;
constexpr uint8_t CODE_LONG_ISLAND = 8;
constexpr int32_t AVENUE_SPACING = 11;
constexpr int32_t STREET_SPACING = 5;
constexpr int32_t ROCKAWAY_QUEENS_FIX_MIN_Y = 304;

bool isInsideBakedCell(int32_t x, int32_t y) {
    return x >= 0 && y >= 0 && x < BOROUGH_MAP_SIZE && y < BOROUGH_MAP_SIZE;
}

uint8_t readBakedCell(const std::vector<uint8_t>& bakedRegion, int32_t x, int32_t y) {
    return bakedRegion[static_cast<size_t>(y) * BOROUGH_MAP_SIZE + x];
}

bool isBakedCellAdjacentToCode(const std::vector<uint8_t>& bakedRegion, int32_t x, int32_t y, uint8_t code) {
    const int32_t neighborOffsets[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    for (int32_t offsetIndex = 0; offsetIndex < 4; ++offsetIndex) {
        const int32_t neighborX = x + neighborOffsets[offsetIndex][0];
        const int32_t neighborY = y + neighborOffsets[offsetIndex][1];
        if (!isInsideBakedCell(neighborX, neighborY)) {
            continue;
        }
        if (readBakedCell(bakedRegion, neighborX, neighborY) == code) {
            return true;
        }
    }
    return false;
}

void enqueueLongIslandBakedCell(std::vector<WorldCoord>& queue, const std::vector<uint8_t>& bakedRegion, int32_t x, int32_t y) {
    if (!isInsideBakedCell(x, y)) {
        return;
    }
    if (readBakedCell(bakedRegion, x, y) != CODE_LONG_ISLAND) {
        return;
    }
    queue.push_back(WorldCoord{x, y});
}

RegionId regionForCode(uint8_t code) {
    switch (code) {
    case 1: return RegionId::Manhattan;
    case 2: return RegionId::Brooklyn;
    case 3: return RegionId::Queens;
    case 4: return RegionId::Bronx;
    case 5: return RegionId::StatenIsland;
    case 6: return RegionId::NewJersey;
    case 7: return RegionId::NewJersey;
    case 8: return RegionId::LongIsland;
    default: return RegionId::None;
    }
}
} // namespace

void WorldGenerator::generate(const WorldConfig& worldConfig, ChunkStore& chunkStore, uint64_t inputSeed) {
    worldSeed = inputSeed;
    worldWidth = worldConfig.WORLD_WIDTH_TILES;
    worldHeight = worldConfig.WORLD_HEIGHT_TILES;
    decodeBakedMap();
    applyBoroughMaskCorrections();
    passBoroughs(chunkStore);
    passStreets(chunkStore);
    passElevation(chunkStore);
}

void WorldGenerator::decodeBakedMap() {
    const size_t cellCount = static_cast<size_t>(BOROUGH_MAP_SIZE) * static_cast<size_t>(BOROUGH_MAP_SIZE);
    bakedRegion.assign(cellCount, CODE_WATER);
    size_t writeIndex = 0;
    for (int32_t pair = 0; pair < BOROUGH_MAP_RLE_LEN; ++pair) {
        const uint8_t code = BOROUGH_MAP_RLE_CODE[pair];
        const uint16_t run = BOROUGH_MAP_RLE_RUN[pair];
        for (uint16_t step = 0; step < run && writeIndex < cellCount; ++step) {
            bakedRegion[writeIndex] = code;
            ++writeIndex;
        }
    }
}

uint8_t WorldGenerator::sampleBakedCode(int32_t x, int32_t y) const {
    const int32_t cellX = x * BOROUGH_MAP_SIZE / worldWidth;
    const int32_t cellY = y * BOROUGH_MAP_SIZE / worldHeight;
    const int32_t clampedX = cellX < 0 ? 0 : (cellX >= BOROUGH_MAP_SIZE ? BOROUGH_MAP_SIZE - 1 : cellX);
    const int32_t clampedY = cellY < 0 ? 0 : (cellY >= BOROUGH_MAP_SIZE ? BOROUGH_MAP_SIZE - 1 : cellY);
    return bakedRegion[static_cast<size_t>(clampedY) * BOROUGH_MAP_SIZE + clampedX];
}

void WorldGenerator::applyBoroughMaskCorrections() {
    std::vector<WorldCoord> fillQueue;
    fillQueue.reserve(6144);
    for (int32_t y = ROCKAWAY_QUEENS_FIX_MIN_Y; y < BOROUGH_MAP_SIZE; ++y) {
        for (int32_t x = 0; x < BOROUGH_MAP_SIZE; ++x) {
            if (readBakedCell(bakedRegion, x, y) != CODE_LONG_ISLAND) {
                continue;
            }
            if (!isBakedCellAdjacentToCode(bakedRegion, x, y, CODE_QUEENS)) {
                continue;
            }
            fillQueue.push_back(WorldCoord{x, y});
        }
    }
    for (size_t queueIndex = 0; queueIndex < fillQueue.size(); ++queueIndex) {
        const WorldCoord cell = fillQueue[queueIndex];
        if (cell.y < ROCKAWAY_QUEENS_FIX_MIN_Y) {
            continue;
        }
        const size_t index = static_cast<size_t>(cell.y) * BOROUGH_MAP_SIZE + cell.x;
        if (bakedRegion[index] != CODE_LONG_ISLAND) {
            continue;
        }
        bakedRegion[index] = CODE_QUEENS;
        enqueueLongIslandBakedCell(fillQueue, bakedRegion, cell.x + 1, cell.y);
        enqueueLongIslandBakedCell(fillQueue, bakedRegion, cell.x - 1, cell.y);
        enqueueLongIslandBakedCell(fillQueue, bakedRegion, cell.x, cell.y + 1);
        enqueueLongIslandBakedCell(fillQueue, bakedRegion, cell.x, cell.y - 1);
    }
}

void WorldGenerator::passBoroughs(ChunkStore& chunkStore) {
    for (int32_t y = 0; y < worldHeight; ++y) {
        for (int32_t x = 0; x < worldWidth; ++x) {
            const WorldCoord coord{x, y};
            const uint8_t code = sampleBakedCode(x, y);
            if (code == CODE_WATER) {
                chunkStore.setTerrainAt(coord, TerrainId::Water);
                chunkStore.setRegionAt(coord, RegionId::None);
                continue;
            }
            const bool isMainland = code >= CODE_NEW_JERSEY;
            chunkStore.setTerrainAt(coord, isMainland ? TerrainId::OpenLand : TerrainId::Building);
            chunkStore.setRegionAt(coord, regionForCode(code));
        }
    }
}

void WorldGenerator::passStreets(ChunkStore& chunkStore) {
    for (int32_t y = 0; y < worldHeight; ++y) {
        for (int32_t x = 0; x < worldWidth; ++x) {
            const WorldCoord coord{x, y};
            if (chunkStore.getTerrainAt(coord) != TerrainId::Building) {
                continue;
            }
            const bool isAvenue = (x % AVENUE_SPACING) == 0;
            const bool isStreet = (y % STREET_SPACING) == 0;
            if (isAvenue || isStreet) {
                chunkStore.setTerrainAt(coord, TerrainId::Road);
            }
        }
    }
}



void WorldGenerator::passElevation(ChunkStore& chunkStore) {
    for (int32_t y = 0; y < worldHeight; ++y) {
        for (int32_t x = 0; x < worldWidth; ++x) {
            const WorldCoord coord{x, y};
            const TerrainId terrainId = chunkStore.getTerrainAt(coord);
            if (terrainId == TerrainId::Water) {
                chunkStore.setElevationAt(coord, 0);
                continue;
            }
            if (terrainId == TerrainId::OpenLand) {
                chunkStore.setElevationAt(coord, 8);
                continue;
            }
            if (terrainId == TerrainId::Park || terrainId == TerrainId::Plaza) {
                chunkStore.setElevationAt(coord, 4);
                continue;
            }
            if (terrainId == TerrainId::Road) {
                chunkStore.setElevationAt(coord, 2);
                continue;
            }
            int16_t baseHeight = 16;
            switch (chunkStore.getRegionAt(coord)) {
            case RegionId::Manhattan: baseHeight = 70; break;
            case RegionId::Brooklyn: baseHeight = 30; break;
            case RegionId::Queens: baseHeight = 28; break;
            case RegionId::Bronx: baseHeight = 34; break;
            case RegionId::StatenIsland: baseHeight = 18; break;
            default: baseHeight = 16; break;
            }
            const uint32_t hash = Utils::hashSeedMix(worldSeed, x, y);
            chunkStore.setElevationAt(coord, static_cast<int16_t>(baseHeight + static_cast<int16_t>(hash % 16u)));
        }
    }
}

} // namespace Core
