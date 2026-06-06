#include "game/npc_spatial_init.h"
#include "game/property_store.h"
#include "sim/character_agent.h"
#include "world/business_node_table.h"
#include "world/chunk_store.h"

namespace Core {

namespace {

constexpr int32_t NPC_VEHICLE_CASH_THRESHOLD_CENTS = 15000;
constexpr int32_t NPC_BICYCLE_CASH_THRESHOLD_CENTS = 2500;
constexpr int32_t NPC_HIGH_CASH_THRESHOLD_CENTS = 200000;
constexpr int32_t NPC_MID_CASH_THRESHOLD_CENTS = 80000;
constexpr int32_t NPC_LOW_CASH_THRESHOLD_CENTS = 30000;
constexpr int32_t NPC_INITIAL_OUT_AND_ABOUT_PERCENT = 40;

} // namespace

MobilityAsset rollNpcMobilityAsset(
    AgentMotive motive,
    int32_t cashCents,
    RegionId regionId,
    uint64_t worldSeed,
    int32_t agentIndex) {
    const uint64_t seed = worldSeed ^ static_cast<uint64_t>(agentIndex) * 0x9E37ULL;
    const uint32_t roll = static_cast<uint32_t>(seed % 100);
    int32_t vehicleThreshold = 8;
    int32_t bicycleThreshold = 25;
    if (motive == AgentMotive::Wealth) {
        vehicleThreshold += 15;
    }
    if (motive == AgentMotive::Status) {
        vehicleThreshold += 10;
    }
    if (motive == AgentMotive::Survival) {
        vehicleThreshold -= 5;
        bicycleThreshold -= 5;
    }
    if (regionId == RegionId::StatenIsland || regionId == RegionId::NewJersey) {
        vehicleThreshold += 8;
    }
    if (regionId == RegionId::Manhattan) {
        vehicleThreshold -= 5;
        bicycleThreshold += 5;
    }
    if (cashCents >= NPC_HIGH_CASH_THRESHOLD_CENTS) {
        vehicleThreshold += 20;
    } else if (cashCents >= NPC_MID_CASH_THRESHOLD_CENTS) {
        vehicleThreshold += 10;
    } else if (cashCents < NPC_LOW_CASH_THRESHOLD_CENTS) {
        vehicleThreshold -= 10;
        bicycleThreshold -= 10;
    }
    if (roll < static_cast<uint32_t>(vehicleThreshold) && cashCents >= NPC_VEHICLE_CASH_THRESHOLD_CENTS) {
        return MobilityAsset::Vehicle;
    }
    if (roll < static_cast<uint32_t>(bicycleThreshold) && cashCents >= NPC_BICYCLE_CASH_THRESHOLD_CENTS) {
        return MobilityAsset::Bicycle;
    }
    return MobilityAsset::None;
}

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

int32_t findCommunityTenementInRegion(const PropertyStore& propertyStore, RegionId regionId, uint64_t seed) {
    int32_t matchCount = 0;
    for (int32_t propertyIndex = 0; propertyIndex < propertyStore.recordCount; ++propertyIndex) {
        const PropertyRecord& record = propertyStore.records[propertyIndex];
        if (record.ownershipType != PropertyOwnershipType::NpcOwned || record.ownerAgentIndex >= 0) {
            continue;
        }
        if (regionId != RegionId::None && static_cast<RegionId>(record.regionId) != regionId) {
            continue;
        }
        matchCount += 1;
    }
    if (matchCount <= 0) {
        return -1;
    }
    const int32_t pick = static_cast<int32_t>(seed % static_cast<uint64_t>(matchCount));
    int32_t seen = 0;
    for (int32_t propertyIndex = 0; propertyIndex < propertyStore.recordCount; ++propertyIndex) {
        const PropertyRecord& record = propertyStore.records[propertyIndex];
        if (record.ownershipType != PropertyOwnershipType::NpcOwned || record.ownerAgentIndex >= 0) {
            continue;
        }
        if (regionId != RegionId::None && static_cast<RegionId>(record.regionId) != regionId) {
            continue;
        }
        if (seen == pick) {
            return propertyIndex;
        }
        seen += 1;
    }
    return -1;
}

bool tryAssignNpcSharedHome(
    CharacterAgentState& agentState,
    const PropertyStore& propertyStore,
    RegionId regionPreference,
    uint64_t seed) {
    int32_t tenementIndex = findCommunityTenementInRegion(propertyStore, regionPreference, seed);
    if (tenementIndex < 0) {
        tenementIndex = findCommunityTenementInRegion(propertyStore, RegionId::None, seed >> 1);
    }
    if (tenementIndex < 0) {
        return false;
    }
    const PropertyRecord* tenement = getPropertyRecord(propertyStore, tenementIndex);
    if (tenement == nullptr) {
        return false;
    }
    agentState.homePropertyIndex = tenementIndex;
    setAgentPosition(agentState, tenement->tileX, tenement->tileY);
    setAgentActivity(agentState, AgentActivity::AtHome, 0);
    return true;
}

void tryAssignNpcWorkplace(
    CharacterAgentState& agentState,
    const PropertyStore& propertyStore,
    int32_t agentIndex,
    uint64_t worldSeed) {
    if (agentState.homePropertyIndex < 0) {
        return;
    }
    const PropertyRecord* homeProperty = getPropertyRecord(propertyStore, agentState.homePropertyIndex);
    if (homeProperty == nullptr) {
        return;
    }
    const int32_t businessCount = getBusinessNodeCount();
    int32_t matchCount = 0;
    for (int32_t businessIndex = 0; businessIndex < businessCount; ++businessIndex) {
        const BusinessNodeDefinition* business = getBusinessNodeDefinition(businessIndex);
        if (business == nullptr || business->kind != BusinessNodeKind::Employer) {
            continue;
        }
        if (getBusinessNodeRegionId(businessIndex) != static_cast<RegionId>(homeProperty->regionId)) {
            continue;
        }
        matchCount += 1;
    }
    if (matchCount <= 0) {
        return;
    }
    const uint32_t pickIndex = static_cast<uint32_t>((worldSeed ^ static_cast<uint64_t>(agentIndex) * 0x5742ULL) % static_cast<uint32_t>(matchCount));
    int32_t seenMatches = 0;
    for (int32_t businessIndex = 0; businessIndex < businessCount; ++businessIndex) {
        const BusinessNodeDefinition* business = getBusinessNodeDefinition(businessIndex);
        if (business == nullptr || business->kind != BusinessNodeKind::Employer) {
            continue;
        }
        if (getBusinessNodeRegionId(businessIndex) != static_cast<RegionId>(homeProperty->regionId)) {
            continue;
        }
        if (seenMatches == static_cast<int32_t>(pickIndex)) {
            agentState.workplaceBusinessIndex = businessIndex;
            return;
        }
        seenMatches += 1;
    }
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

        const RegionId regionPreference = static_cast<RegionId>(state.homeRegionId);
        const uint64_t homeSeed = worldSeed ^ (static_cast<uint64_t>(agentIndex) * 0x2545F4914F6CDD1DULL);
        if (!tryAssignNpcSharedHome(state, propertyStore, regionPreference, homeSeed)) {
            tryAssignNpcHome(state, propertyStore, agentIndex, preferredKind);
        }
        tryAssignNpcWorkplace(state, propertyStore, agentIndex, worldSeed);
        RegionId homeRegionId = RegionId::None;
        if (state.homePropertyIndex >= 0) {
            const PropertyRecord* homeProperty = getPropertyRecord(propertyStore, state.homePropertyIndex);
            if (homeProperty != nullptr) {
                homeRegionId = static_cast<RegionId>(homeProperty->regionId);
            }
        }
        state.mobilityAsset = rollNpcMobilityAsset(
            state.generatedMotive,
            state.cashCents,
            homeRegionId,
            worldSeed,
            agentIndex);

        // Seed initial liveliness: a fraction of the generated population starts
        // out and about (Idle, visible on the map) so the city looks alive at
        // game start; the autonomy loop takes over from there.
        if (state.hasGeneratedIdentity && state.currentTileX >= 0) {
            const uint32_t liveRoll = static_cast<uint32_t>(
                (worldSeed ^ (static_cast<uint64_t>(agentIndex) * 0x9E3779B97F4A7C15ULL)) % 100ULL);
            if (liveRoll < static_cast<uint32_t>(NPC_INITIAL_OUT_AND_ABOUT_PERCENT)) {
                setAgentActivity(state, AgentActivity::Idle, 0);
            }
        }
    }
}

} // namespace Core
