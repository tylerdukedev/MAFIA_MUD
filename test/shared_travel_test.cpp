#include "game/shared_travel_state.h"
#include "game/npc_spatial_init.h"
#include "sim/character_agent.h"
#include "world/chunk_store.h"
#include "world/world_config.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace Core;

namespace {

void paintRoadLine(ChunkStore& chunkStore, int32_t startX, int32_t startY, int32_t endX, int32_t endY) {
    const int32_t stepX = startX <= endX ? 1 : -1;
    const int32_t stepY = startY <= endY ? 1 : -1;
    int32_t tileX = startX;
    int32_t tileY = startY;
    while (tileX != endX || tileY != endY) {
        chunkStore.setTerrainAt(WorldCoord{tileX, tileY}, TerrainId::Road);
        if (tileX != endX) {
            tileX += stepX;
        } else {
            tileY += stepY;
        }
    }
    chunkStore.setTerrainAt(WorldCoord{endX, endY}, TerrainId::Road);
}

} // namespace

TEST_CASE("resolveTravelModeForMobility maps assets to travel modes", "[shared_travel]") {
    REQUIRE(resolveTravelModeForMobility(MobilityAsset::None) == TravelMode::Walk);
    REQUIRE(resolveTravelModeForMobility(MobilityAsset::Bicycle) == TravelMode::Bicycle);
    REQUIRE(resolveTravelModeForMobility(MobilityAsset::Vehicle) == TravelMode::Car);
}

TEST_CASE("rollNpcMobilityAsset is deterministic from seed", "[shared_travel]") {
    const MobilityAsset firstRoll = rollNpcMobilityAsset(
        AgentMotive::Wealth,
        250000,
        RegionId::StatenIsland,
        4242,
        3);
    const MobilityAsset secondRoll = rollNpcMobilityAsset(
        AgentMotive::Wealth,
        250000,
        RegionId::StatenIsland,
        4242,
        3);
    REQUIRE(firstRoll == secondRoll);
}

TEST_CASE("copyRoadPathToAgentTravel caps at NPC_MAX_TRAVEL_PATH_TILES", "[shared_travel]") {
    CharacterAgentState agent{};
    RoadPathResult roadPath{};
    roadPath.tileCount = NPC_MAX_TRAVEL_PATH_TILES + 4;
    for (int32_t pathIndex = 0; pathIndex < roadPath.tileCount; ++pathIndex) {
        roadPath.tileX[pathIndex] = pathIndex;
        roadPath.tileY[pathIndex] = pathIndex;
    }
    copyRoadPathToAgentTravel(agent, roadPath);
    REQUIRE(agent.travelPathTileCount == NPC_MAX_TRAVEL_PATH_TILES);
    REQUIRE(agent.travelPathTileX[0] == 0);
    REQUIRE(agent.travelPathTileY[NPC_MAX_TRAVEL_PATH_TILES - 1] == NPC_MAX_TRAVEL_PATH_TILES - 1);
}

TEST_CASE("Agent road travel advances along path and completes", "[shared_travel]") {
    WorldConfig worldConfig{};
    ChunkStore chunkStore{worldConfig};
    paintRoadLine(chunkStore, 10, 10, 14, 10);
    CharacterAgentState agent{};
    agent.isActive = true;
    setAgentPosition(agent, 10, 10);
    agent.mobilityAsset = MobilityAsset::None;
    const uint64_t startTick = 100;
    const bool travelStarted = beginAgentRoadTravel(agent, 14, 10, startTick, chunkStore, worldConfig);
    REQUIRE(travelStarted);
    REQUIRE(agent.currentActivity == AgentActivity::Traveling);
    REQUIRE(agent.travelPathTileCount > 1);
    tickAgentTravel(agent, startTick + 1, nullptr);
    REQUIRE(agent.currentActivity == AgentActivity::Traveling);
    tickAgentTravel(agent, agent.travelCompleteTick, nullptr);
    REQUIRE(agent.currentActivity == AgentActivity::Idle);
    REQUIRE(agent.currentTileX == 14);
    REQUIRE(agent.currentTileY == 10);
    REQUIRE(agent.travelPathTileCount == 0);
}

TEST_CASE("computeAgentTravelVisualT interpolates between start and complete", "[shared_travel]") {
    CharacterAgentState agent{};
    agent.currentActivity = AgentActivity::Traveling;
    agent.travelStartTick = 100;
    agent.travelCompleteTick = 200;
    REQUIRE(computeAgentTravelVisualT(agent, 100) == Catch::Approx(0.0f));
    REQUIRE(computeAgentTravelVisualT(agent, 150) == Catch::Approx(0.5f));
    REQUIRE(computeAgentTravelVisualT(agent, 200) == Catch::Approx(1.0f));
}

TEST_CASE("getAgentDisplayTile samples path while traveling", "[shared_travel]") {
    CharacterAgentState agent{};
    agent.currentActivity = AgentActivity::Traveling;
    agent.travelPathTileCount = 2;
    agent.travelPathTileX[0] = 0;
    agent.travelPathTileY[0] = 0;
    agent.travelPathTileX[1] = 4;
    agent.travelPathTileY[1] = 0;
    agent.travelStartTick = 0;
    agent.travelCompleteTick = 100;
    float displayX = 0.0f;
    float displayY = 0.0f;
    getAgentDisplayTile(agent, 50, displayX, displayY);
    REQUIRE(displayX == Catch::Approx(2.5f));
    REQUIRE(displayY == Catch::Approx(0.5f));
}
