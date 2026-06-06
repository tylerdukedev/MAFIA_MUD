#pragma once

#include "game/road_pathfinding.h"
#include "game/travel_modes.h"
#include "world/chunk_store.h"
#include "world/world_config.h"
#include <cstdint>

namespace Core {

struct CharacterAgentState;
struct CharacterAgentStore;
struct PropertyStore;

constexpr int32_t NPC_MAX_TRAVEL_PATH_TILES = 128;

enum class MobilityAsset : uint8_t {
    None = 0,
    Bicycle = 1,
    Vehicle = 2,
};

void copyRoadPathToAgentTravel(CharacterAgentState& agent, const RoadPathResult& roadPath);
TravelMode resolveTravelModeForMobility(MobilityAsset mobilityAsset);
bool beginAgentRoadTravel(
    CharacterAgentState& agent,
    int32_t destTileX,
    int32_t destTileY,
    uint64_t tickCount,
    const ChunkStore& chunkStore,
    const WorldConfig& worldConfig);
void tickAgentTravel(CharacterAgentState& agent, uint64_t tickCount, const PropertyStore* propertyStore);
float computeAgentTravelVisualT(const CharacterAgentState& agent, uint64_t tickCount);
void getAgentDisplayTile(const CharacterAgentState& agent, uint64_t tickCount, float& outTileX, float& outTileY);
void tickAllAgentTravel(
    CharacterAgentStore& agentStore,
    uint64_t tickCount,
    const PropertyStore* propertyStore);

} // namespace Core
