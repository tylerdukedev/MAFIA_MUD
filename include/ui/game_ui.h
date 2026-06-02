#pragma once

#include "core/sim_clock.h"
#include "sim/system_registry.h"
#include "ui/viewport_state.h"
#include "world/chunk_store.h"
#include "world/world_config.h"

namespace Core {

void renderGameUi(
    SimClock& simClock,
    const WorldConfig& worldConfig,
    ChunkStore& chunkStore,
    SystemRegistry& systemRegistry,
    ViewportPickState& viewportPickState);

} // namespace Core
