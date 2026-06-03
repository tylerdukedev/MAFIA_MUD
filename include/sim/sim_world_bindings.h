#pragma once

#include "world/chunk_store.h"
#include "world/tile_vitality.h"
#include "world/world_config.h"
#include <cstdint>

namespace Core {

struct SimWorldBindings {
    ChunkStore* chunkStore = nullptr;
    BoroughVitalityStore* boroughVitalityStore = nullptr;
    const WorldConfig* worldConfig = nullptr;
    uint64_t* worldSeed = nullptr;
};

} // namespace Core
