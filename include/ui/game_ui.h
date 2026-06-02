#pragma once

#include "core/sim_clock.h"
#include "sim/system_registry.h"
#include "ui/map_camera.h"
#include "ui/viewport_state.h"
#include "world/chunk_store.h"
#include "world/world_config.h"
#include <cstdint>

namespace Core {

void renderGameUi(
    SimClock& simClock,
    const WorldConfig& worldConfig,
    const ChunkStore& chunkStore,
    SystemRegistry& systemRegistry,
    MapCamera& mapCamera,
    ViewportPickState& viewportPickState,
    uint64_t worldSeed);

} // namespace Core
