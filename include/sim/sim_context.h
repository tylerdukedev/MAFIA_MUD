#pragma once

#include "world/chunk_store.h"
#include "world/district_store.h"
#include "world/tile_field_store.h"
#include "world/world_config.h"

namespace Core {

struct SimContext {
    const WorldConfig* worldConfig = nullptr;
    const ChunkStore* chunkStore = nullptr;
    TileFieldStore* tileFieldStore = nullptr;
    DistrictStore* districtStore = nullptr;
};

} // namespace Core
