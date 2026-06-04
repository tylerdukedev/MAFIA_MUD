#pragma once

#include "game/property_store.h"
#include "world/chunk_store.h"
#include <cstdint>

namespace Core {

// Generate property records at residential locations across the map
void generateProperties(
    PropertyStore& propertyStore,
    const ChunkStore& chunkStore,
    uint64_t worldSeed);

} // namespace Core
