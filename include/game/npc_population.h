#pragma once

#include "sim/agent_relationship_graph.h"
#include "sim/character_agent.h"
#include <cstdint>

namespace Core {

struct PropertyStore;
class ChunkStore;

constexpr int32_t NPC_POPULATION_FIRST_SLOT_INDEX = 7;
constexpr int32_t NPC_POPULATION_TARGET_COUNT = 72;

// Places shared "tenement" residence records that many NPCs can live in.
// Kept separate from for-sale listings so the player market stays intact.
void generateNpcTenements(PropertyStore& propertyStore, const ChunkStore& chunkStore, uint64_t worldSeed);

// Activates and identity-stamps a diverse city population in slots
// [NPC_POPULATION_FIRST_SLOT_INDEX, MAX_CHARACTER_AGENT_COUNT). Spatial,
// home/work, cash and mobility assignment is done afterward by
// initializeNpcSpatialState.
void generateCityPopulation(CharacterAgentStore& agentStore, uint64_t worldSeed);

// Builds NPC-to-NPC ties (households, coworkers, criminal associates, rivalries)
// from the final spatial/home/work assignment.
void generateNpcRelationships(
    AgentRelationshipGraph& graph,
    const CharacterAgentStore& agentStore,
    const PropertyStore& propertyStore,
    uint64_t worldSeed);

AgentArchetype rollAgentArchetype(uint32_t roll);
AgentMotive motiveForArchetype(AgentArchetype archetype, uint32_t roll);
AgentPersonalityTrait traitForArchetype(AgentArchetype archetype, uint32_t roll);

} // namespace Core
