#pragma once

#include "character/player_profile.h"
#include "game/player_law_enforcement.h"
#include "game/player_organization.h"
#include "game/player_operations.h"
#include "game/player_wallet.h"
#include "game/street_crime.h"
#include "sim/character_agent.h"
#include "sim/sim_event_queue.h"
#include "sim/world_event_store.h"
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
    PlayerOrganizationStore* playerOrganizationStore = nullptr;
    PlayerLawEnforcementStore* playerLawEnforcementStore = nullptr;
    PlayerStreetCrimeStore* playerStreetCrimeStore = nullptr;
    CharacterAgentStore* characterAgentStore = nullptr;
    WorldEventStore* worldEventStore = nullptr;
};

} // namespace Core
