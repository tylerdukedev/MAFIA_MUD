#pragma once

#include "core/types.h"
#include "game/property_store.h"
#include "game/shared_travel_state.h"
#include "sim/character_agent.h"
#include "world/chunk_store.h"
#include <cstdint>

namespace Core {

// Initialize active agents with homes, jobs, and starting cash
void initializeNpcSpatialState(
    CharacterAgentStore& agentStore,
    PropertyStore& propertyStore,
    const ChunkStore& chunkStore,
    uint64_t worldSeed);

// Assign an available property to an NPC and set their home position
bool tryAssignNpcHome(
    CharacterAgentState& agentState,
    PropertyStore& propertyStore,
    int32_t agentIndex,
    HeadquartersKind preferredKind);

// Generate starting cash for NPC based on their background
int32_t generateNpcStartingCash(AgentMotive motive, uint64_t worldSeed, int32_t agentIndex);

// Roll mobility asset from motive, cash, region, and seed
MobilityAsset rollNpcMobilityAsset(
    AgentMotive motive,
    int32_t cashCents,
    RegionId regionId,
    uint64_t worldSeed,
    int32_t agentIndex);

} // namespace Core
