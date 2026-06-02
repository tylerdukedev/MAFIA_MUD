#pragma once

#include "core/sim_clock.h"
#include "world/chunk_store.h"
#include "world/world_config.h"

namespace Core {

void renderGameUi(SimClock& simClock, const WorldConfig& worldConfig, const ChunkStore& chunkStore);

} // namespace Core
