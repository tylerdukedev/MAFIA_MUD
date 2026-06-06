#include "game/npc_population.h"
#include "game/operation_types.h"
#include "game/property_store.h"
#include "game/road_pathfinding.h"
#include "sim/character_agent.h"
#include "utils/seed_hash.h"
#include "world/business_node_table.h"
#include "world/chunk_store.h"
#include "world/region_table.h"
#include "world/world_config.h"
#include <algorithm>
#include <cstdio>
#include <cstring>

namespace Core {

namespace {

constexpr int32_t HOUSEHOLD_CAPACITY = 6;

const char* const FIRST_NAMES[] = {
    "Salvatore", "Vincent", "Angelo", "Maria", "Rosa", "Giovanni", "Patrick",
    "Bridget", "Sean", "Nora", "Moishe", "Rachel", "Abe", "Hyman", "Wei",
    "Mei", "Dmitri", "Anastasia", "Stanislaw", "Wanda", "Frank", "Concetta",
    "Liam", "Saul", "Esther", "Carmine", "Lucia", "Declan", "Yusuf", "Ada",
};
constexpr int32_t FIRST_NAME_COUNT = static_cast<int32_t>(sizeof(FIRST_NAMES) / sizeof(FIRST_NAMES[0]));

const char* const LAST_NAMES[] = {
    "Genovese", "Moretti", "Russo", "Caruso", "O'Brien", "Kelly", "Murphy",
    "Donnelly", "Goldberg", "Rosen", "Levine", "Schwartz", "Chen", "Wong",
    "Petrov", "Volkov", "Kowalski", "Nowak", "Barbieri", "Esposito", "Ferraro",
    "Walsh", "Fitzgerald", "Friedman", "Liang", "Sokolov", "Adamski", "Romano",
    "Hassan", "Mancuso",
};
constexpr int32_t LAST_NAME_COUNT = static_cast<int32_t>(sizeof(LAST_NAMES) / sizeof(LAST_NAMES[0]));

struct RegionTenementPlan {
    RegionId region;
    int32_t count;
};

constexpr RegionTenementPlan REGION_TENEMENTS[] = {
    {RegionId::Manhattan, 8},
    {RegionId::Brooklyn, 8},
    {RegionId::Queens, 6},
    {RegionId::Bronx, 5},
    {RegionId::StatenIsland, 3},
    {RegionId::NewJersey, 3},
};
constexpr int32_t REGION_TENEMENT_PLAN_COUNT = static_cast<int32_t>(sizeof(REGION_TENEMENTS) / sizeof(REGION_TENEMENTS[0]));

bool isTileNearRoad(const ChunkStore& chunkStore, int32_t tileX, int32_t tileY) {
    for (int32_t offsetY = -2; offsetY <= 2; ++offsetY) {
        for (int32_t offsetX = -2; offsetX <= 2; ++offsetX) {
            const WorldCoord coord{tileX + offsetX, tileY + offsetY};
            if (isRoadTraversibleTerrain(chunkStore.getTerrainAt(coord))) {
                return true;
            }
        }
    }
    return false;
}

RegionId pickHomeRegion(uint32_t roll) {
    if (roll < 28U) return RegionId::Manhattan;
    if (roll < 56U) return RegionId::Brooklyn;
    if (roll < 74U) return RegionId::Queens;
    if (roll < 90U) return RegionId::Bronx;
    if (roll < 95U) return RegionId::StatenIsland;
    return RegionId::NewJersey;
}

int32_t rollOpinionBaseline(AgentArchetype archetype, uint32_t roll) {
    if (isLawEnforcementArchetype(archetype)) {
        return -25 + static_cast<int32_t>(roll % 20U);
    }
    if (isCriminalArchetype(archetype)) {
        return -10 + static_cast<int32_t>(roll % 30U);
    }
    return -12 + static_cast<int32_t>(roll % 26U);
}

AgentEmotion baselineEmotionForArchetype(AgentArchetype archetype) {
    if (isCriminalArchetype(archetype)) {
        return AgentEmotion::Suspicious;
    }
    if (isLawEnforcementArchetype(archetype)) {
        return AgentEmotion::Suspicious;
    }
    return AgentEmotion::Calm;
}

void collectResidents(
    const CharacterAgentStore& agentStore,
    int32_t propertyIndex,
    int32_t (&outResidents)[HOUSEHOLD_CAPACITY],
    int32_t& outCount) {
    outCount = 0;
    for (int32_t agentIndex = 0; agentIndex < MAX_CHARACTER_AGENT_COUNT && outCount < HOUSEHOLD_CAPACITY; ++agentIndex) {
        const CharacterAgentState& state = agentStore.states[agentIndex];
        if (!state.isActive || state.homePropertyIndex != propertyIndex) {
            continue;
        }
        outResidents[outCount] = agentIndex;
        outCount += 1;
    }
}

void collectCoworkers(
    const CharacterAgentStore& agentStore,
    int32_t businessIndex,
    int32_t (&outWorkers)[HOUSEHOLD_CAPACITY],
    int32_t& outCount) {
    outCount = 0;
    for (int32_t agentIndex = 0; agentIndex < MAX_CHARACTER_AGENT_COUNT && outCount < HOUSEHOLD_CAPACITY; ++agentIndex) {
        const CharacterAgentState& state = agentStore.states[agentIndex];
        if (!state.isActive || state.workplaceBusinessIndex != businessIndex) {
            continue;
        }
        outWorkers[outCount] = agentIndex;
        outCount += 1;
    }
}

} // namespace

AgentArchetype rollAgentArchetype(uint32_t roll) {
    const uint32_t value = roll % 100U;
    if (value < 25U) return AgentArchetype::Laborer;
    if (value < 38U) return AgentArchetype::Shopkeeper;
    if (value < 47U) return AgentArchetype::Clerk;
    if (value < 55U) return AgentArchetype::Dockworker;
    if (value < 61U) return AgentArchetype::Bartender;
    if (value < 66U) return AgentArchetype::Cabbie;
    if (value < 70U) return AgentArchetype::UnionMan;
    if (value < 73U) return AgentArchetype::Doctor;
    if (value < 76U) return AgentArchetype::Priest;
    if (value < 79U) return AgentArchetype::Patrolman;
    if (value < 81U) return AgentArchetype::Detective;
    if (value < 83U) return AgentArchetype::Politician;
    if (value < 85U) return AgentArchetype::Socialite;
    if (value < 87U) return AgentArchetype::Landlord;
    if (value < 90U) return AgentArchetype::NumbersRunner;
    if (value < 93U) return AgentArchetype::Bootlegger;
    if (value < 95U) return AgentArchetype::Enforcer;
    if (value < 97U) return AgentArchetype::Fence;
    if (value < 98U) return AgentArchetype::Loanshark;
    if (value < 99U) return AgentArchetype::Racketeer;
    return AgentArchetype::Vagrant;
}

AgentMotive motiveForArchetype(AgentArchetype archetype, uint32_t roll) {
    if (isCriminalArchetype(archetype)) {
        return (roll % 100U) < 70U ? AgentMotive::Wealth : AgentMotive::Revenge;
    }
    if (isLawEnforcementArchetype(archetype)) {
        return (roll % 100U) < 50U ? AgentMotive::Survival : AgentMotive::Status;
    }
    switch (archetype) {
        case AgentArchetype::Politician:
        case AgentArchetype::Socialite:
        case AgentArchetype::Landlord:
            return (roll % 100U) < 50U ? AgentMotive::Status : AgentMotive::Wealth;
        case AgentArchetype::Laborer:
        case AgentArchetype::Dockworker:
        case AgentArchetype::Cabbie:
        case AgentArchetype::Vagrant:
            return AgentMotive::Survival;
        case AgentArchetype::Shopkeeper:
        case AgentArchetype::Clerk:
        case AgentArchetype::Bartender:
            return (roll % 100U) < 50U ? AgentMotive::Wealth : AgentMotive::Survival;
        case AgentArchetype::UnionMan:
        case AgentArchetype::Priest:
        case AgentArchetype::Doctor:
            return AgentMotive::Status;
        default:
            return AgentMotive::Loyalty;
    }
}

AgentPersonalityTrait traitForArchetype(AgentArchetype archetype, uint32_t roll) {
    if (isCriminalArchetype(archetype)) {
        return (roll % 100U) < 60U ? AgentPersonalityTrait::Ruthless : AgentPersonalityTrait::Proud;
    }
    if (isLawEnforcementArchetype(archetype)) {
        return (roll % 100U) < 60U ? AgentPersonalityTrait::Pragmatic : AgentPersonalityTrait::Paranoid;
    }
    if (archetype == AgentArchetype::Priest || archetype == AgentArchetype::Doctor) {
        return AgentPersonalityTrait::Charitable;
    }
    if (archetype == AgentArchetype::Politician || archetype == AgentArchetype::Socialite) {
        return AgentPersonalityTrait::Proud;
    }
    const uint32_t pick = roll % 4U;
    switch (pick) {
        case 0: return AgentPersonalityTrait::Pragmatic;
        case 1: return AgentPersonalityTrait::Proud;
        case 2: return AgentPersonalityTrait::Paranoid;
        default: return AgentPersonalityTrait::Charitable;
    }
}

void generateNpcTenements(PropertyStore& propertyStore, const ChunkStore& chunkStore, uint64_t worldSeed) {
    for (int32_t planIndex = 0; planIndex < REGION_TENEMENT_PLAN_COUNT; ++planIndex) {
        const RegionTenementPlan& plan = REGION_TENEMENTS[planIndex];
        int32_t placed = 0;
        const int32_t maxAttempts = plan.count * 40;
        for (int32_t attempt = 0; attempt < maxAttempts && placed < plan.count; ++attempt) {
            const uint32_t hash = Utils::hashSeedMix(worldSeed, static_cast<int32_t>(plan.region) * 7919 + attempt, 0x54454E45);
            const int32_t tileX = static_cast<int32_t>(hash % static_cast<uint32_t>(WorldConfig::WORLD_WIDTH_TILES));
            const int32_t tileY = static_cast<int32_t>((hash / 13U) % static_cast<uint32_t>(WorldConfig::WORLD_HEIGHT_TILES));
            const WorldCoord coord{tileX, tileY};
            if (chunkStore.getRegionAt(coord) != plan.region) {
                continue;
            }
            const TerrainId terrainId = chunkStore.getTerrainAt(coord);
            if (terrainId == TerrainId::Water || terrainId == TerrainId::None) {
                continue;
            }
            if (!isTileNearRoad(chunkStore, tileX, tileY)) {
                continue;
            }
            const int32_t propertyIndex = addPropertyRecord(
                propertyStore,
                tileX,
                tileY,
                HeadquartersKind::RentedRoom,
                static_cast<int32_t>(HQ_RENTED_ROOM_COST_CENTS),
                static_cast<uint8_t>(plan.region));
            if (propertyIndex < 0) {
                continue;
            }
            propertyStore.records[propertyIndex].ownershipType = PropertyOwnershipType::NpcOwned;
            propertyStore.records[propertyIndex].ownerAgentIndex = -1;
            placed += 1;
        }
    }
}

void generateCityPopulation(CharacterAgentStore& agentStore, uint64_t worldSeed) {
    const int32_t lastSlot = std::min(
        NPC_POPULATION_FIRST_SLOT_INDEX + NPC_POPULATION_TARGET_COUNT,
        MAX_CHARACTER_AGENT_COUNT);
    for (int32_t slotIndex = NPC_POPULATION_FIRST_SLOT_INDEX; slotIndex < lastSlot; ++slotIndex) {
        CharacterAgentState& state = agentStore.states[slotIndex];
        const uint32_t hash = Utils::hashSeedMix(worldSeed, slotIndex, 0x4E504350);
        const AgentArchetype archetype = rollAgentArchetype(hash);
        const AgentMotive motive = motiveForArchetype(archetype, hash >> 7);
        const AgentPersonalityTrait trait = traitForArchetype(archetype, hash >> 11);
        const RegionId region = pickHomeRegion((hash >> 17) % 100U);
        const int32_t firstNameIndex = static_cast<int32_t>((hash >> 3) % static_cast<uint32_t>(FIRST_NAME_COUNT));
        const int32_t lastNameIndex = static_cast<int32_t>((hash >> 19) % static_cast<uint32_t>(LAST_NAME_COUNT));
        state = CharacterAgentState{};
        state.isActive = true;
        state.hasGeneratedIdentity = true;
        std::snprintf(
            state.generatedDisplayName,
            sizeof(state.generatedDisplayName),
            "%s %s",
            FIRST_NAMES[firstNameIndex],
            LAST_NAMES[lastNameIndex]);
        std::strncpy(state.generatedRoleLabel, agentArchetypeToLabel(archetype), sizeof(state.generatedRoleLabel) - 1);
        state.generatedArchetype = archetype;
        state.generatedMotive = motive;
        state.generatedTrait = trait;
        state.currentObjective = AgentObjective::Settle;
        state.currentEmotion = baselineEmotionForArchetype(archetype);
        state.homeRegionId = static_cast<uint8_t>(region);
        state.opinionOfPlayer = std::clamp(rollOpinionBaseline(archetype, hash >> 23), AGENT_OPINION_MIN, AGENT_OPINION_MAX);
        deriveRelationshipStatsFromOpinion(state);
    }
}

void generateNpcRelationships(
    AgentRelationshipGraph& graph,
    const CharacterAgentStore& agentStore,
    const PropertyStore& propertyStore,
    uint64_t worldSeed) {
    resetAgentRelationshipGraph(graph);
    for (int32_t propertyIndex = 0; propertyIndex < propertyStore.recordCount; ++propertyIndex) {
        int32_t residents[HOUSEHOLD_CAPACITY];
        int32_t residentCount = 0;
        collectResidents(agentStore, propertyIndex, residents, residentCount);
        if (residentCount < 2) {
            continue;
        }
        const AgentTieKind kind = residentCount <= 3 ? AgentTieKind::Family : AgentTieKind::Friend;
        for (int32_t a = 0; a < residentCount; ++a) {
            for (int32_t b = a + 1; b < residentCount; ++b) {
                const uint32_t affinityHash = Utils::hashSeedMix(worldSeed, residents[a] * 131 + residents[b], 0x484F4D45);
                const int32_t affinity = (kind == AgentTieKind::Family ? 45 : 25) + static_cast<int32_t>(affinityHash % 26U);
                linkAgentsBidirectional(graph, residents[a], residents[b], kind, affinity);
            }
        }
    }
    const int32_t businessCount = getBusinessNodeCount();
    for (int32_t businessIndex = 0; businessIndex < businessCount; ++businessIndex) {
        int32_t workers[HOUSEHOLD_CAPACITY];
        int32_t workerCount = 0;
        collectCoworkers(agentStore, businessIndex, workers, workerCount);
        if (workerCount < 2) {
            continue;
        }
        for (int32_t a = 0; a < workerCount; ++a) {
            for (int32_t b = a + 1; b < workerCount; ++b) {
                const uint32_t affinityHash = Utils::hashSeedMix(worldSeed, workers[a] * 251 + workers[b], 0x574F524B);
                const int32_t affinity = 12 + static_cast<int32_t>(affinityHash % 28U);
                linkAgentsBidirectional(graph, workers[a], workers[b], AgentTieKind::Coworker, affinity);
            }
        }
    }
    int32_t lastCriminal = -1;
    for (int32_t agentIndex = 0; agentIndex < MAX_CHARACTER_AGENT_COUNT; ++agentIndex) {
        const CharacterAgentState& state = agentStore.states[agentIndex];
        if (!state.isActive || !isCriminalArchetype(state.generatedArchetype)) {
            continue;
        }
        if (lastCriminal >= 0) {
            const uint32_t pairHash = Utils::hashSeedMix(worldSeed, lastCriminal * 97 + agentIndex, 0x52495641);
            if ((pairHash % 100U) < 35U) {
                linkAgentsBidirectional(graph, lastCriminal, agentIndex, AgentTieKind::Rival, -30 - static_cast<int32_t>(pairHash % 30U));
            } else {
                linkAgentsBidirectional(graph, lastCriminal, agentIndex, AgentTieKind::Associate, 20 + static_cast<int32_t>(pairHash % 30U));
            }
        }
        lastCriminal = agentIndex;
    }
}

} // namespace Core
