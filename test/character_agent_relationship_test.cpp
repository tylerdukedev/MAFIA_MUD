#include <catch2/catch_test_macros.hpp>
#include "sim/character_agent.h"

namespace Core {

TEST_CASE("deriveRelationshipStatsFromOpinion keeps rivals low trust", "[character_agent]") {
    CharacterAgentState state{};
    state.opinionOfPlayer = -40;
    deriveRelationshipStatsFromOpinion(state);
    REQUIRE(state.trust < 15);
    REQUIRE(state.respect < state.trust + 25);
}

TEST_CASE("deriveRelationshipStatsFromOpinion raises allies", "[character_agent]") {
    CharacterAgentState state{};
    state.opinionOfPlayer = 50;
    deriveRelationshipStatsFromOpinion(state);
    REQUIRE(state.trust >= 50);
    REQUIRE(state.respect >= 40);
}

} // namespace Core
