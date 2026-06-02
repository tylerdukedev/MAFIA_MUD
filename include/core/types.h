#pragma once

#include <cstdint>

namespace Core {

using TileId = uint32_t;
using ChunkId = uint32_t;

constexpr TileId INVALID_TILE_ID = UINT32_MAX;
constexpr ChunkId INVALID_CHUNK_ID = UINT32_MAX;

struct WorldCoord {
    int32_t x = 0;
    int32_t y = 0;
};

struct ChunkCoord {
    int32_t x = 0;
    int32_t y = 0;
};

struct LocalTileCoord {
    uint16_t x = 0;
    uint16_t y = 0;
};

enum class RegionId : uint8_t {
    None = 0,
    Manhattan,
    Brooklyn,
    Queens,
    Bronx,
    StatenIsland,
    NewJersey,
    COUNT
};

enum class TerrainId : uint8_t {
    None = 0,
    Water,
    Land,
    COUNT
};

} // namespace Core
