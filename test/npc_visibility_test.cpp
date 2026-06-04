#include "sim/character_agent.h"
#include "game/property_store.h"
#include "game/npc_spatial_init.h"
#include <catch2/catch_test_macros.hpp>

using namespace Core;

TEST_CASE("NPC visibility changes with activity", "[npc_spatial]") {
    CharacterAgentState agent{};
    agent.isActive = true;

    SECTION("NPC at home is not visible") {
        setAgentActivity(agent, AgentActivity::AtHome, 0);
        REQUIRE(isAgentVisibleOnMap(agent) == false);
    }

    SECTION("NPC traveling is visible") {
        setAgentActivity(agent, AgentActivity::Traveling, 100);
        REQUIRE(isAgentVisibleOnMap(agent) == true);
    }

    SECTION("NPC at work is not visible") {
        setAgentActivity(agent, AgentActivity::AtWork, 200);
        REQUIRE(isAgentVisibleOnMap(agent) == false);
    }

    SECTION("Transition from home to traveling makes NPC visible") {
        setAgentActivity(agent, AgentActivity::AtHome, 0);
        REQUIRE(isAgentVisibleOnMap(agent) == false);

        setAgentActivity(agent, AgentActivity::Traveling, 100);
        REQUIRE(isAgentVisibleOnMap(agent) == true);
    }

    SECTION("Transition from traveling to work hides NPC") {
        setAgentActivity(agent, AgentActivity::Traveling, 0);
        REQUIRE(isAgentVisibleOnMap(agent) == true);

        setAgentActivity(agent, AgentActivity::AtWork, 100);
        REQUIRE(isAgentVisibleOnMap(agent) == false);
    }
}

TEST_CASE("NPC home assignment sets position", "[npc_spatial]") {
    CharacterAgentState agent{};
    agent.isActive = true;
    agent.generatedMotive = AgentMotive::Survival;

    PropertyStore propertyStore{};

    // Add a test property
    const int32_t propertyIndex = addPropertyRecord(
        propertyStore,
        100,  // tileX
        200,  // tileY
        HeadquartersKind::RentedRoom,
        2200,
        static_cast<uint8_t>(RegionId::Manhattan));

    REQUIRE(propertyIndex >= 0);

    // Assign home to NPC
    const bool assigned = tryAssignNpcHome(agent, propertyStore, 0, HeadquartersKind::RentedRoom);
    REQUIRE(assigned == true);

    // Check NPC position was set to property location
    REQUIRE(agent.currentTileX == 100);
    REQUIRE(agent.currentTileY == 200);
    REQUIRE(agent.homePropertyIndex == propertyIndex);
    REQUIRE(agent.currentActivity == AgentActivity::AtHome);
    REQUIRE(isAgentVisibleOnMap(agent) == false);
}

TEST_CASE("Multiple NPCs can have different properties", "[npc_spatial]") {
    PropertyStore propertyStore{};

    // Add two properties
    addPropertyRecord(propertyStore, 100, 200, HeadquartersKind::RentedRoom, 2200, 1);
    addPropertyRecord(propertyStore, 150, 250, HeadquartersKind::Apartment, 4200, 1);

    CharacterAgentState agent1{};
    agent1.isActive = true;

    CharacterAgentState agent2{};
    agent2.isActive = true;

    // Assign different homes
    REQUIRE(tryAssignNpcHome(agent1, propertyStore, 0, HeadquartersKind::RentedRoom) == true);
    REQUIRE(tryAssignNpcHome(agent2, propertyStore, 1, HeadquartersKind::Apartment) == true);

    // Each NPC should be at their own property
    REQUIRE(agent1.currentTileX == 100);
    REQUIRE(agent1.currentTileY == 200);
    REQUIRE(agent2.currentTileX == 150);
    REQUIRE(agent2.currentTileY == 250);

    // Property ownership should be tracked
    const PropertyRecord* prop1 = getPropertyRecord(propertyStore, 0);
    const PropertyRecord* prop2 = getPropertyRecord(propertyStore, 1);

    REQUIRE(prop1->ownershipType == PropertyOwnershipType::NpcOwned);
    REQUIRE(prop1->ownerAgentIndex == 0);
    REQUIRE(prop2->ownershipType == PropertyOwnershipType::NpcOwned);
    REQUIRE(prop2->ownerAgentIndex == 1);
}
