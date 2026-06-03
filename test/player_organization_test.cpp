#include "character/character_social_network.h"
#include "character/profile_builder.h"
#include "game/street_crime.h"
#include "game/player_law_enforcement.h"
#include "game/player_organization.h"
#include "game/player_wallet.h"
#include "sim/character_agent.h"
#include <catch2/catch_test_macros.hpp>

using namespace Core;

TEST_CASE("Crew recruit locks when trust is below threshold", "[player_organization]") {
    CharacterAgentStore agents{};
    initializeCharacterAgentStore(agents);
    CharacterAgentState& friendState = agents.states[FRIEND_AGENT_SLOT_INDEX];
    friendState.isActive = true;
    friendState.hasGeneratedIdentity = true;
    friendState.opinionOfPlayer = 15;
    friendState.trust = 30;
    friendState.respect = 50;
    PlayerOrganizationStore organization{};
    const CrewRecruitLockReason lockReason = evaluateCrewRecruitLock(organization, agents, FRIEND_AGENT_SLOT_INDEX);
    REQUIRE(lockReason == CrewRecruitLockReason::LowTrust);
}

TEST_CASE("Formalize crew requires two recruited members", "[player_organization]") {
    PlayerOrganizationStore organization{};
    organization.crewMemberCount = 1;
    organization.crewMemberAgentIndices[0] = FRIEND_AGENT_SLOT_INDEX;
    REQUIRE_FALSE(tryFormalizeCrew(organization, "Test Crew", 50ULL));
    organization.crewMemberAgentIndices[1] = RIVAL_AGENT_SLOT_INDEX;
    organization.crewMemberCount = 2;
    REQUIRE(tryFormalizeCrew(organization, "Test Crew", 50ULL));
    REQUIRE(organization.powerTier == PlayerPowerTier::Crew);
}

TEST_CASE("Organization crimes require organization tier", "[player_organization]") {
    REQUIRE(meetsStreetCrimeTierRequirement(PlayerPowerTier::Solo, StreetCrimeTier::Organization) == false);
    REQUIRE(meetsStreetCrimeTierRequirement(PlayerPowerTier::Organization, StreetCrimeTier::Organization));
}
