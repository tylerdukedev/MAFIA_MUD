#pragma once

#include "character/player_profile.h"
#include "game/player_operations.h"
#include "game/player_wallet.h"
#include "sim/sim_event_queue.h"
#include "world/chunk_store.h"
#include "world/city_control.h"
#include "world/tile_vitality.h"
#include "world/world_config.h"
#include <cstdint>

namespace Core {

struct SimWorldBindings {
    ChunkStore* chunkStore = nullptr;
    BoroughVitalityStore* boroughVitalityStore = nullptr;
    const WorldConfig* worldConfig = nullptr;
    uint64_t* worldSeed = nullptr;
    PlayerWallet* playerWallet = nullptr;
    CityControlStore* cityControlStore = nullptr;
    SimEventQueue* eventQueue = nullptr;
    PlayerProfile* playerProfile = nullptr;
    PlayerOperationsStore* playerOperationsStore = nullptr;
};

} // namespace Core
