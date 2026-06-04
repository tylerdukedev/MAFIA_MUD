#include "game/npc_spatial_init.h"
#include "game/property_store.h"
#include "sim/character_agent.h"
#include "world/chunk_store.h"

namespace Core {

int32_t generateNpcStartingCash(AgentMotive motive, uint64_t worldSeed, int32_t agentIndex) {
    const uint64_t seed = worldSeed ^ static_cast<uint64_t>(agentIndex);
    int32_t baseCash = 0;

    switch (motive) {
        case AgentMotive::Wealth:
            baseCash = 8000 + static_cast<int32_t>((seed % 12000));
            break;
        case AgentMotive::Status:
            baseCash = 6000 + static_cast<int32_t>((seed % 8000));
            break;
        case AgentMotive::Survival:
            baseCash = 3000 + static_cast<int32_t>((seed % 4000));
            break;
        case AgentMotive::Loyalty:
            baseCash = 4000 + static_cast<int32_t>((seed % 6000));
            break;
        case AgentMotive::Revenge:
            baseCash = 5000 + static_cast<int32_t>((seed % 7000));
            break;
        default:
            baseCash = 5000;
            break;
    }

    return baseCash;
}

bool tryAssignNpcHome(
    CharacterAgentState& agentState,
    PropertyStore& propertyStore,
    int32_t agentIndex,
    HeadquartersKind preferredKind) {

    // Find available property
    const int32_t propertyIndex = findAvailableProperty(propertyStore, preferredKind);
    if (propertyIndex < 0) {
        return false;
    }

    // Assign property to NPC
    if (!tryAssignPropertyToNpc(propertyStore, propertyIndex, agentIndex)) {
        return false;
    }

    const PropertyRecord* property = getPropertyRecord(propertyStore, propertyIndex);
    if (property == nullptr) {
        return false;
    }

    // Set home reference and position
    agentState.homePropertyIndex = propertyIndex;
    setAgentPosition(agentState, property->tileX, property->tileY);
    setAgentActivity(agentState, AgentActivity::AtHome, 0);

    return true;
}

void initializeNpcSpatialState(
    CharacterAgentStore& agentStore,
    PropertyStore& propertyStore,
    const ChunkStore& chunkStore,
    uint64_t worldSeed) {

    // Initialize each active agent
    for (int32_t agentIndex = 0; agentIndex < MAX_CHARACTER_AGENT_COUNT; ++agentIndex) {
        CharacterAgentState& state = agentStore.states[agentIndex];

        if (!state.isActive) {
            continue;
        }

        // Generate starting cash based on motive
        state.cashCents = generateNpcStartingCash(state.generatedMotive, worldSeed, agentIndex);

        // Assign home (prefer rented room for most NPCs, apartment for wealthy)
        HeadquartersKind preferredKind = HeadquartersKind::RentedRoom;
        if (state.generatedMotive == AgentMotive::Wealth) {
            preferredKind = HeadquartersKind::Apartment;
        }

        tryAssignNpcHome(state, propertyStore, agentIndex, preferredKind);

        // NPCs start at home (not visible on map initially)
        // They will become visible when they start traveling/working in future phases
    }
}

} // namespace Core
