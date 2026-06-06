#include "game/shared_travel_state.h"
#include "game/property_store.h"
#include "sim/character_agent.h"
#include "world/business_node_table.h"
#include <algorithm>
#include <cmath>

namespace Core {

namespace {

void clearAgentTravelPath(CharacterAgentState& agent) {
    agent.travelPathTileCount = 0;
}

bool tryGetPropertyTile(const PropertyStore& propertyStore, int32_t propertyIndex, int32_t& outTileX, int32_t& outTileY) {
    const PropertyRecord* property = getPropertyRecord(propertyStore, propertyIndex);
    if (property == nullptr || property->tileX < 0 || property->tileY < 0) {
        return false;
    }
    outTileX = property->tileX;
    outTileY = property->tileY;
    return true;
}

AgentActivity resolveArrivalActivity(const CharacterAgentState& agent, const PropertyStore* propertyStore) {
    if (propertyStore != nullptr && agent.homePropertyIndex >= 0) {
        int32_t homeTileX = -1;
        int32_t homeTileY = -1;
        if (tryGetPropertyTile(*propertyStore, agent.homePropertyIndex, homeTileX, homeTileY)
            && agent.currentTileX == homeTileX
            && agent.currentTileY == homeTileY) {
            return AgentActivity::AtHome;
        }
    }
    if (agent.workplaceBusinessIndex >= 0) {
        const BusinessNodeDefinition* workplace = getBusinessNodeDefinition(agent.workplaceBusinessIndex);
        if (workplace != nullptr
            && agent.currentTileX == workplace->tileX
            && agent.currentTileY == workplace->tileY) {
            return AgentActivity::AtWork;
        }
    }
    return AgentActivity::Idle;
}

int32_t resolveTravelPathIndex(const CharacterAgentState& agent, float travelT) {
    if (agent.travelPathTileCount <= 1) {
        return 0;
    }
    const int32_t maxPathIndex = agent.travelPathTileCount - 1;
    const float pathProgress = travelT * static_cast<float>(maxPathIndex);
    int32_t pathIndex = static_cast<int32_t>(pathProgress);
    if (pathIndex < 0) {
        pathIndex = 0;
    }
    if (pathIndex > maxPathIndex) {
        pathIndex = maxPathIndex;
    }
    return pathIndex;
}

void syncTravelLogicalTile(CharacterAgentState& agent, float travelT) {
    if (agent.travelPathTileCount <= 0) {
        return;
    }
    const int32_t pathIndex = resolveTravelPathIndex(agent, travelT);
    agent.currentTileX = agent.travelPathTileX[pathIndex];
    agent.currentTileY = agent.travelPathTileY[pathIndex];
}

void sampleTravelPathTile(
    const CharacterAgentState& agent,
    float travelT,
    float& outTileX,
    float& outTileY) {
    if (agent.travelPathTileCount <= 0) {
        outTileX = static_cast<float>(agent.currentTileX) + 0.5f;
        outTileY = static_cast<float>(agent.currentTileY) + 0.5f;
        return;
    }
    if (agent.travelPathTileCount == 1) {
        outTileX = static_cast<float>(agent.travelPathTileX[0]) + 0.5f;
        outTileY = static_cast<float>(agent.travelPathTileY[0]) + 0.5f;
        return;
    }
    const int32_t lowerIndex = resolveTravelPathIndex(agent, travelT);
    const int32_t upperIndex = lowerIndex >= agent.travelPathTileCount - 1 ? lowerIndex : lowerIndex + 1;
    const float pathProgress = travelT * static_cast<float>(agent.travelPathTileCount - 1);
    const float segmentT = pathProgress - static_cast<float>(lowerIndex);
    const float startX = static_cast<float>(agent.travelPathTileX[lowerIndex]) + 0.5f;
    const float startY = static_cast<float>(agent.travelPathTileY[lowerIndex]) + 0.5f;
    const float endX = static_cast<float>(agent.travelPathTileX[upperIndex]) + 0.5f;
    const float endY = static_cast<float>(agent.travelPathTileY[upperIndex]) + 0.5f;
    outTileX = startX + (endX - startX) * segmentT;
    outTileY = startY + (endY - startY) * segmentT;
}

} // namespace

void copyRoadPathToAgentTravel(CharacterAgentState& agent, const RoadPathResult& roadPath) {
    clearAgentTravelPath(agent);
    if (roadPath.tileCount <= 0) {
        return;
    }
    const int32_t copyCount = std::min(roadPath.tileCount, NPC_MAX_TRAVEL_PATH_TILES);
    agent.travelPathTileCount = copyCount;
    for (int32_t pathIndex = 0; pathIndex < copyCount; ++pathIndex) {
        agent.travelPathTileX[pathIndex] = roadPath.tileX[pathIndex];
        agent.travelPathTileY[pathIndex] = roadPath.tileY[pathIndex];
    }
}

TravelMode resolveTravelModeForMobility(MobilityAsset mobilityAsset) {
    switch (mobilityAsset) {
    case MobilityAsset::Bicycle:
        return TravelMode::Bicycle;
    case MobilityAsset::Vehicle:
        return TravelMode::Car;
    case MobilityAsset::None:
    default:
        return TravelMode::Walk;
    }
}

bool beginAgentRoadTravel(
    CharacterAgentState& agent,
    int32_t destTileX,
    int32_t destTileY,
    uint64_t tickCount,
    const ChunkStore& chunkStore,
    const WorldConfig& worldConfig) {
    if (agent.currentActivity == AgentActivity::Traveling && agent.travelPathTileCount > 0) {
        return false;
    }
    if (agent.currentTileX == destTileX && agent.currentTileY == destTileY) {
        agent.travelDestTileX = destTileX;
        agent.travelDestTileY = destTileY;
        agent.destinationTileX = -1;
        agent.destinationTileY = -1;
        clearAgentTravelPath(agent);
        return true;
    }
    RoadPathResult roadPath{};
    if (!findRoadPath(
            chunkStore,
            worldConfig,
            agent.currentTileX,
            agent.currentTileY,
            destTileX,
            destTileY,
            roadPath)) {
        return false;
    }
    if (roadPath.tileCount > NPC_MAX_TRAVEL_PATH_TILES) {
        return false;
    }
    const TravelMode travelMode = resolveTravelModeForMobility(agent.mobilityAsset);
    TravelPlan plan{};
    buildTravelPlan(
        plan,
        travelMode,
        agent.currentTileX,
        agent.currentTileY,
        destTileX,
        destTileY,
        chunkStore,
        worldConfig);
    const int32_t durationTicks = plan.estimatedTicks < 1 ? 1 : plan.estimatedTicks;
    copyRoadPathToAgentTravel(agent, roadPath);
    if (agent.travelPathTileCount < 1) {
        return false;
    }
    agent.travelMode = travelMode;
    agent.travelDestTileX = agent.travelPathTileX[agent.travelPathTileCount - 1];
    agent.travelDestTileY = agent.travelPathTileY[agent.travelPathTileCount - 1];
    agent.destinationTileX = destTileX;
    agent.destinationTileY = destTileY;
    agent.currentTileX = agent.travelPathTileX[0];
    agent.currentTileY = agent.travelPathTileY[0];
    agent.travelStartTick = tickCount;
    agent.travelCompleteTick = tickCount + static_cast<uint64_t>(durationTicks);
    setAgentActivity(agent, AgentActivity::Traveling, tickCount);
    return true;
}

void tickAgentTravel(CharacterAgentState& agent, uint64_t tickCount, const PropertyStore* propertyStore) {
    if (agent.currentActivity != AgentActivity::Traveling) {
        return;
    }
    if (agent.travelPathTileCount <= 0) {
        setAgentActivity(agent, AgentActivity::Idle, tickCount);
        return;
    }
    const float travelT = computeAgentTravelVisualT(agent, tickCount);
    syncTravelLogicalTile(agent, travelT);
    if (tickCount < agent.travelCompleteTick) {
        return;
    }
    agent.currentTileX = agent.travelDestTileX;
    agent.currentTileY = agent.travelDestTileY;
    agent.destinationTileX = -1;
    agent.destinationTileY = -1;
    clearAgentTravelPath(agent);
    setAgentActivity(agent, resolveArrivalActivity(agent, propertyStore), tickCount);
}

float computeAgentTravelVisualT(const CharacterAgentState& agent, uint64_t tickCount) {
    if (agent.currentActivity != AgentActivity::Traveling) {
        return 1.0f;
    }
    if (agent.travelCompleteTick <= agent.travelStartTick) {
        return 1.0f;
    }
    if (tickCount >= agent.travelCompleteTick) {
        return 1.0f;
    }
    const float elapsed = static_cast<float>(tickCount - agent.travelStartTick);
    const float span = static_cast<float>(agent.travelCompleteTick - agent.travelStartTick);
    return elapsed / span;
}

void getAgentDisplayTile(const CharacterAgentState& agent, uint64_t tickCount, float& outTileX, float& outTileY) {
    if (agent.currentActivity != AgentActivity::Traveling) {
        outTileX = static_cast<float>(agent.currentTileX) + 0.5f;
        outTileY = static_cast<float>(agent.currentTileY) + 0.5f;
        return;
    }
    const float travelT = computeAgentTravelVisualT(agent, tickCount);
    sampleTravelPathTile(agent, travelT, outTileX, outTileY);
}

void tickAllAgentTravel(
    CharacterAgentStore& agentStore,
    uint64_t tickCount,
    const PropertyStore* propertyStore) {
    for (int32_t agentIndex = 0; agentIndex < MAX_CHARACTER_AGENT_COUNT; ++agentIndex) {
        tickAgentTravel(agentStore.states[agentIndex], tickCount, propertyStore);
    }
}

} // namespace Core
