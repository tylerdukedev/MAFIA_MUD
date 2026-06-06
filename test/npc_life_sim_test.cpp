#include <catch2/catch_test_macros.hpp>

#include "game/housing_living_costs.h"
#include "game/npc_decision.h"
#include "game/npc_population.h"
#include "game/property_store.h"
#include "sim/agent_relationship_graph.h"
#include "sim/character_agent.h"
#include "world/business_node_table.h"
#include <algorithm>

using namespace Core;

namespace {

int32_t findEmployerWithWage(int64_t& outWageCents) {
    const int32_t businessCount = getBusinessNodeCount();
    for (int32_t businessIndex = 0; businessIndex < businessCount; ++businessIndex) {
        const BusinessNodeDefinition* business = getBusinessNodeDefinition(businessIndex);
        if (business != nullptr && business->jobWageCents > 0) {
            outWageCents = business->jobWageCents;
            return businessIndex;
        }
    }
    return -1;
}

} // namespace

TEST_CASE("Relationship ties are bidirectional, deduped, and capacity-bounded", "[npc][relationships]") {
    AgentRelationshipGraph inputGraph{};
    resetAgentRelationshipGraph(inputGraph);

    linkAgentsBidirectional(inputGraph, 7, 8, AgentTieKind::Family, 40);

    REQUIRE(getAgentTieCount(inputGraph, 7) == 1);
    REQUIRE(getAgentTieCount(inputGraph, 8) == 1);
    const AgentTie* forwardTie = getAgentTie(inputGraph, 7, 0);
    REQUIRE(forwardTie != nullptr);
    REQUIRE(forwardTie->targetAgentIndex == 8);
    REQUIRE(forwardTie->kind == static_cast<uint8_t>(AgentTieKind::Family));
    REQUIRE(forwardTie->affinity == 40);

    // Re-linking the same pair updates in place rather than duplicating.
    addAgentTie(inputGraph, 7, 8, AgentTieKind::Friend, 10);
    REQUIRE(getAgentTieCount(inputGraph, 7) == 1);
    REQUIRE(getAgentTie(inputGraph, 7, 0)->kind == static_cast<uint8_t>(AgentTieKind::Friend));

    // Capacity is bounded at AGENT_MAX_TIES.
    for (int32_t target = 10; target < 10 + AGENT_MAX_TIES + 4; ++target) {
        addAgentTie(inputGraph, 7, target, AgentTieKind::Associate, 5);
    }
    REQUIRE(getAgentTieCount(inputGraph, 7) == AGENT_MAX_TIES);
}

TEST_CASE("Archetype distribution is mostly working civilians with a criminal minority", "[npc][archetype]") {
    int32_t criminalCount = 0;
    int32_t lawCount = 0;
    for (uint32_t roll = 0; roll < 100U; ++roll) {
        const AgentArchetype archetype = rollAgentArchetype(roll);
        if (isCriminalArchetype(archetype)) {
            criminalCount += 1;
        }
        if (isLawEnforcementArchetype(archetype)) {
            lawCount += 1;
        }
    }

    REQUIRE(criminalCount >= 8);
    REQUIRE(criminalCount <= 20);
    REQUIRE(lawCount >= 1);

    REQUIRE(motiveForArchetype(AgentArchetype::Racketeer, 0U) == AgentMotive::Wealth);
    REQUIRE(traitForArchetype(AgentArchetype::Racketeer, 0U) == AgentPersonalityTrait::Ruthless);
}

TEST_CASE("City population generation activates a diverse identity-stamped population", "[npc][population]") {
    CharacterAgentStore inputStore{};
    generateCityPopulation(inputStore, 0x1234ABCDULL);

    int32_t activeCount = 0;
    for (int32_t agentIndex = NPC_POPULATION_FIRST_SLOT_INDEX; agentIndex < MAX_CHARACTER_AGENT_COUNT; ++agentIndex) {
        const CharacterAgentState& state = inputStore.states[agentIndex];
        if (!state.isActive) {
            continue;
        }
        activeCount += 1;
        REQUIRE(state.hasGeneratedIdentity);
        REQUIRE(state.generatedDisplayName[0] != '\0');
        REQUIRE(static_cast<uint8_t>(state.generatedArchetype) < static_cast<uint8_t>(AgentArchetype::Count));
    }

    REQUIRE(activeCount >= 60);
}

TEST_CASE("NPC monthly ledger pays wages net of rent and living costs", "[npc][economy]") {
    int64_t wageCents = 0;
    const int32_t employerIndex = findEmployerWithWage(wageCents);
    REQUIRE(employerIndex >= 0);

    PropertyStore propertyStore{};
    resetPropertyStore(propertyStore);
    const int32_t monthlyRentCents = 1400;
    const int32_t homeIndex = addPropertyRecord(propertyStore, 10, 10, HeadquartersKind::RentedRoom, monthlyRentCents, 1);
    REQUIRE(homeIndex >= 0);

    CharacterAgentStore agentStore{};
    CharacterAgentState& worker = agentStore.states[NPC_POPULATION_FIRST_SLOT_INDEX];
    worker = CharacterAgentState{};
    worker.isActive = true;
    worker.hasGeneratedIdentity = true;
    worker.generatedArchetype = AgentArchetype::Laborer;
    worker.workplaceBusinessIndex = employerIndex;
    worker.homePropertyIndex = homeIndex;
    worker.cashCents = 1000;
    worker.lastWageTick = 0;

    const uint64_t payTick = static_cast<uint64_t>(MONTHLY_LEDGER_INTERVAL_TICKS) + 5ULL;
    tickNpcMonthlyLedger(agentStore, propertyStore, payTick, 0x99ULL);

    const int64_t expectedIncome = wageCents * static_cast<int64_t>(JOB_MONTHLY_WAGE_MULTIPLIER);
    const int64_t expectedCash = std::clamp<int64_t>(1000 + expectedIncome - monthlyRentCents - 600, 0, 5000000);
    REQUIRE(static_cast<int64_t>(worker.cashCents) == expectedCash);
    REQUIRE(worker.lastWageTick == payTick);

    // A second pass inside the same month must not pay again.
    const int32_t cashAfterFirst = worker.cashCents;
    tickNpcMonthlyLedger(agentStore, propertyStore, payTick + 10ULL, 0x99ULL);
    REQUIRE(worker.cashCents == cashAfterFirst);
}
