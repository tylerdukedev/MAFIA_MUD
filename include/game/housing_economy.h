#pragma once

#include "game/player_operations.h"
#include "world/chunk_store.h"
#include "world/tile_vitality.h"
#include <cstdint>

namespace Core {

int32_t computeHousingRentMultiplierBps(
    const BoroughVitalityStore& boroughVitalityStore,
    const ChunkStore& chunkStore,
    uint8_t headquartersRegionId);
void updatePlayerHousingRentMultiplier(
    PlayerOperationsStore& operationsStore,
    const BoroughVitalityStore& boroughVitalityStore,
    const ChunkStore& chunkStore);

} // namespace Core
