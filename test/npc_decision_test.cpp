#include "game/npc_decision.h"
#include "sim/character_agent.h"
#include "game/game_calendar.h"
#include <catch2/catch_test_macros.hpp>

using namespace Core;

TEST_CASE("NPC decision context", "[npc_decision]") {
    NPCDecisionContext context{};
    context.worldSeed = 12345;
    context.tickCount = 1000;
    context.calendarStore = nullptr;

    REQUIRE(context.worldSeed == 12345);
    REQUIRE(context.tickCount == 1000);
    REQUIRE(context.calendarStore == nullptr);
}

TEST_CASE("Activity display labels", "[npc_decision]") {
    REQUIRE(strcmp(getActivityDisplayLabel(AgentActivity::Idle), "Idle") == 0);
    REQUIRE(strcmp(getActivityDisplayLabel(AgentActivity::AtHome), "At Home") == 0);
    REQUIRE(strcmp(getActivityDisplayLabel(AgentActivity::AtWork), "At Work") == 0);
    REQUIRE(strcmp(getActivityDisplayLabel(AgentActivity::Traveling), "Traveling") == 0);
    REQUIRE(strcmp(getActivityDisplayLabel(AgentActivity::InBuilding), "In Building") == 0);
    REQUIRE(strcmp(getActivityDisplayLabel(AgentActivity::Abroad), "Abroad") == 0);
    REQUIRE(strcmp(getActivityDisplayLabel(AgentActivity::Incarcerated), "Incarcerated") == 0);
}

TEST_CASE("NPC at home transitions to visible when traveling", "[npc_decision]") {
    CharacterAgentState agent{};
    agent.isActive = true;
    setAgentActivity(agent, AgentActivity::AtHome, 0);
    REQUIRE(isAgentVisibleOnMap(agent) == false);

    setAgentActivity(agent, AgentActivity::Traveling, 100);
    REQUIRE(isAgentVisibleOnMap(agent) == true);

    // Travel to destination
    agent.destinationTileX = agent.currentTileX + 1;
    agent.destinationTileY = agent.currentTileY;

    // Simulate one tick of travel
    agent.currentTileX += 1;
    if (agent.currentTileX == agent.destinationTileX && agent.currentTileY == agent.destinationTileY) {
        setAgentActivity(agent, AgentActivity::Idle, 100);
    }

    REQUIRE(agent.currentActivity == AgentActivity::Idle);
    REQUIRE(isAgentVisibleOnMap(agent) == true);
}