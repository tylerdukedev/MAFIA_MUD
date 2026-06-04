#include "sim/character_agent.h"
#include "character/character_social_network.h"
#include <catch2/catch_test_macros.hpp>

using namespace Core;

TEST_CASE("Agent spatial state initializes correctly", "[character_agent]") {
    CharacterAgentState state{};

    REQUIRE(state.currentTileX == -1);
    REQUIRE(state.currentTileY == -1);
    REQUIRE(state.currentActivity == AgentActivity::Idle);
    REQUIRE(state.homePropertyIndex == -1);
    REQUIRE(state.workplaceBusinessIndex == -1);
    REQUIRE(state.cashCents == 0);
    REQUIRE(state.isVisibleOnMap == false);
}

TEST_CASE("setAgentPosition updates coordinates", "[character_agent]") {
    CharacterAgentState state{};
    state.isActive = true;

    setAgentPosition(state, 100, 200);

    REQUIRE(state.currentTileX == 100);
    REQUIRE(state.currentTileY == 200);
}

TEST_CASE("setAgentActivity updates activity and visibility", "[character_agent]") {
    CharacterAgentState state{};
    state.isActive = true;

    SECTION("Idle makes agent visible") {
        setAgentActivity(state, AgentActivity::Idle, 100);
        REQUIRE(state.currentActivity == AgentActivity::Idle);
        REQUIRE(state.activityStartTick == 100);
        REQUIRE(state.isVisibleOnMap == true);
    }

    SECTION("Traveling makes agent visible") {
        setAgentActivity(state, AgentActivity::Traveling, 200);
        REQUIRE(state.currentActivity == AgentActivity::Traveling);
        REQUIRE(state.isVisibleOnMap == true);
    }

    SECTION("AtHome hides agent from map") {
        setAgentActivity(state, AgentActivity::AtHome, 300);
        REQUIRE(state.currentActivity == AgentActivity::AtHome);
        REQUIRE(state.isVisibleOnMap == false);
    }

    SECTION("AtWork hides agent from map") {
        setAgentActivity(state, AgentActivity::AtWork, 400);
        REQUIRE(state.currentActivity == AgentActivity::AtWork);
        REQUIRE(state.isVisibleOnMap == false);
    }

    SECTION("InBuilding hides agent from map") {
        setAgentActivity(state, AgentActivity::InBuilding, 500);
        REQUIRE(state.currentActivity == AgentActivity::InBuilding);
        REQUIRE(state.isVisibleOnMap == false);
    }

    SECTION("Abroad hides agent from map") {
        setAgentActivity(state, AgentActivity::Abroad, 600);
        REQUIRE(state.currentActivity == AgentActivity::Abroad);
        REQUIRE(state.isVisibleOnMap == false);
    }

    SECTION("Incarcerated hides agent from map") {
        setAgentActivity(state, AgentActivity::Incarcerated, 700);
        REQUIRE(state.currentActivity == AgentActivity::Incarcerated);
        REQUIRE(state.isVisibleOnMap == false);
    }
}

TEST_CASE("isAgentVisibleOnMap respects active state", "[character_agent]") {
    CharacterAgentState state{};

    SECTION("Inactive agent is never visible") {
        state.isActive = false;
        state.currentActivity = AgentActivity::Traveling;
        state.isVisibleOnMap = true;

        REQUIRE(isAgentVisibleOnMap(state) == false);
    }

    SECTION("Active traveling agent is visible") {
        state.isActive = true;
        setAgentActivity(state, AgentActivity::Traveling, 100);

        REQUIRE(isAgentVisibleOnMap(state) == true);
    }
}

TEST_CASE("Agent store preserves spatial state through reset", "[character_agent]") {
    CharacterAgentStore store{};
    initializeCharacterAgentStore(store);

    // Modify spatial state of an active agent
    store.states[FIRST_COMMUNITY_AGENT_SLOT_INDEX + 1].currentTileX = 150;
    store.states[FIRST_COMMUNITY_AGENT_SLOT_INDEX + 1].currentTileY = 250;
    store.states[FIRST_COMMUNITY_AGENT_SLOT_INDEX + 1].cashCents = 50000;

    REQUIRE(store.states[FIRST_COMMUNITY_AGENT_SLOT_INDEX + 1].currentTileX == 150);
    REQUIRE(store.states[FIRST_COMMUNITY_AGENT_SLOT_INDEX + 1].currentTileY == 250);
    REQUIRE(store.states[FIRST_COMMUNITY_AGENT_SLOT_INDEX + 1].cashCents == 50000);

    // After reset, spatial state should be cleared
    resetCharacterAgentStore(store);

    REQUIRE(store.states[FIRST_COMMUNITY_AGENT_SLOT_INDEX + 1].currentTileX == -1);
    REQUIRE(store.states[FIRST_COMMUNITY_AGENT_SLOT_INDEX + 1].currentTileY == -1);
    REQUIRE(store.states[FIRST_COMMUNITY_AGENT_SLOT_INDEX + 1].cashCents == 0);
}
