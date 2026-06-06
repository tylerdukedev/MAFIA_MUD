#include "game/property_worldgen.h"
#include "game/economy_index.h"
#include "game/operation_types.h"
#include "game/road_pathfinding.h"
#include "utils/seed_hash.h"
#include "world/business_node_table.h"
#include "world/landmark_table.h"
#include "world/world_config.h"
#include <algorithm>

namespace Core {

namespace {

constexpr int32_t LISTINGS_PER_BOROUGH = 12;
constexpr int32_t NODE_COLLISION_DISTANCE_TILES = 3;
constexpr int32_t MAX_PLACEMENT_ATTEMPTS_PER_BOROUGH = 240;

struct TierPricing {
    PropertyListingTier tier;
    int64_t baseAskPriceCents;
    int64_t baseRentCents;
    uint8_t beds;
    HeadquartersKind propertyKind;
};

constexpr TierPricing TIER_PRICING_TABLE[] = {
    {PropertyListingTier::Room, 150000, 1400, 1, HeadquartersKind::RentedRoom},
    {PropertyListingTier::Apartment, 350000, 2800, 2, HeadquartersKind::Apartment},
    {PropertyListingTier::House, 550000, 4200, 4, HeadquartersKind::Apartment},
    {PropertyListingTier::MultiFamily, 800000, 6500, 6, HeadquartersKind::Apartment},
};

constexpr int32_t TIER_PRICING_COUNT = static_cast<int32_t>(sizeof(TIER_PRICING_TABLE) / sizeof(TIER_PRICING_TABLE[0]));

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

bool isTileBlockedByNode(
    const ChunkStore& chunkStore,
    const PropertyStore& propertyStore,
    const PropertyListingStore& listingStore,
    int32_t tileX,
    int32_t tileY) {
    const int32_t minDistanceSquared = NODE_COLLISION_DISTANCE_TILES * NODE_COLLISION_DISTANCE_TILES;
    const int32_t landmarkCount = getLandmarkCount();
    for (int32_t landmarkIndex = 0; landmarkIndex < landmarkCount; ++landmarkIndex) {
        const LandmarkDefinition* landmark = getLandmarkDefinition(landmarkIndex);
        if (landmark == nullptr) {
            continue;
        }
        const int32_t deltaX = landmark->tileX - tileX;
        const int32_t deltaY = landmark->tileY - tileY;
        if (deltaX * deltaX + deltaY * deltaY <= minDistanceSquared) {
            return true;
        }
    }
    const int32_t businessCount = getBusinessNodeCount();
    for (int32_t businessIndex = 0; businessIndex < businessCount; ++businessIndex) {
        const BusinessNodeDefinition* business = getBusinessNodeDefinition(businessIndex);
        if (business == nullptr) {
            continue;
        }
        const int32_t deltaX = business->tileX - tileX;
        const int32_t deltaY = business->tileY - tileY;
        if (deltaX * deltaX + deltaY * deltaY <= minDistanceSquared) {
            return true;
        }
    }
    for (int32_t propertyIndex = 0; propertyIndex < propertyStore.recordCount; ++propertyIndex) {
        const PropertyRecord& record = propertyStore.records[propertyIndex];
        const int32_t deltaX = record.tileX - tileX;
        const int32_t deltaY = record.tileY - tileY;
        if (deltaX * deltaX + deltaY * deltaY <= minDistanceSquared) {
            return true;
        }
    }
    if (isPropertyListingTileOccupied(listingStore, tileX, tileY, NODE_COLLISION_DISTANCE_TILES)) {
        return true;
    }
    const WorldCoord coord{tileX, tileY};
    const TerrainId terrainId = chunkStore.getTerrainAt(coord);
    if (terrainId == TerrainId::Water || terrainId == TerrainId::None) {
        return true;
    }
    (void)terrainId;
    return false;
}

PropertyListingTier pickTierForRoll(uint32_t roll) {
    if (roll < 40U) {
        return PropertyListingTier::Room;
    }
    if (roll < 75U) {
        return PropertyListingTier::Apartment;
    }
    if (roll < 92U) {
        return PropertyListingTier::House;
    }
    return PropertyListingTier::MultiFamily;
}

uint8_t rollPerks(uint32_t hash, bool nearRoad) {
    uint8_t perks = 0;
    if (nearRoad && (hash % 3U) == 0U) {
        perks |= static_cast<uint8_t>(PropertyListingPerks::NearTransit);
    }
    if ((hash % 5U) == 0U) {
        perks |= static_cast<uint8_t>(PropertyListingPerks::Garage);
    }
    if ((hash % 7U) == 0U) {
        perks |= static_cast<uint8_t>(PropertyListingPerks::LowHeat);
    }
    if ((hash % 11U) == 0U) {
        perks |= static_cast<uint8_t>(PropertyListingPerks::FamilySpace);
    }
    return perks;
}

const TierPricing* findTierPricing(PropertyListingTier tier) {
    for (int32_t index = 0; index < TIER_PRICING_COUNT; ++index) {
        if (TIER_PRICING_TABLE[index].tier == tier) {
            return &TIER_PRICING_TABLE[index];
        }
    }
    return &TIER_PRICING_TABLE[0];
}

} // namespace

void generatePropertyListingsForNewGame(
    PropertyListingStore& listingStore,
    PropertyStore& propertyStore,
    const ChunkStore& chunkStore,
    const BoroughVitalityStore& boroughVitalityStore,
    const GameCalendarStore& calendarStore,
    uint64_t worldSeed) {
    resetPropertyListingStore(listingStore);
    const RegionId targetRegions[] = {
        RegionId::Manhattan,
        RegionId::Brooklyn,
        RegionId::Queens,
        RegionId::Bronx,
        RegionId::StatenIsland,
        RegionId::NewJersey,
    };
    for (const RegionId regionId : targetRegions) {
        int32_t placedInRegion = 0;
        for (int32_t attempt = 0; attempt < MAX_PLACEMENT_ATTEMPTS_PER_BOROUGH && placedInRegion < LISTINGS_PER_BOROUGH; ++attempt) {
            const uint32_t hash = Utils::hashSeedMix(worldSeed, static_cast<int32_t>(regionId) * 1000 + attempt, 8801);
            const int32_t tileX = static_cast<int32_t>(hash % static_cast<uint32_t>(WorldConfig::WORLD_WIDTH_TILES));
            const int32_t tileY = static_cast<int32_t>((hash / 17U) % static_cast<uint32_t>(WorldConfig::WORLD_HEIGHT_TILES));
            const WorldCoord coord{tileX, tileY};
            if (chunkStore.getRegionAt(coord) != regionId) {
                continue;
            }
            if (isTileBlockedByNode(chunkStore, propertyStore, listingStore, tileX, tileY)) {
                continue;
            }
            const bool nearRoad = isTileNearRoad(chunkStore, tileX, tileY);
            if (!nearRoad && (hash % 4U) != 0U) {
                continue;
            }
            const PropertyListingTier tier = pickTierForRoll(hash % 100U);
            const TierPricing* pricing = findTierPricing(tier);
            const int32_t economyBps = getCombinedEconomyMultiplierBps(regionId, calendarStore, boroughVitalityStore);
            int64_t askPriceCents = scalePriceCentsByEconomy(pricing->baseAskPriceCents, economyBps);
            int64_t rentCents = scalePriceCentsByEconomy(pricing->baseRentCents, economyBps);
            if ((hash % 9U) == 0U) {
                askPriceCents = (askPriceCents * 105) / 100;
            }
            const uint8_t perks = rollPerks(hash, nearRoad);
            if ((perks & static_cast<uint8_t>(PropertyListingPerks::Garage)) != 0) {
                askPriceCents = (askPriceCents * 108) / 100;
            }
            const uint8_t regionIdValue = static_cast<uint8_t>(regionId);
            const int32_t propertyIndex = addPropertyRecord(
                propertyStore,
                tileX,
                tileY,
                pricing->propertyKind,
                static_cast<int32_t>(rentCents),
                regionIdValue);
            if (propertyIndex < 0) {
                continue;
            }
            addPropertyListing(
                listingStore,
                tileX,
                tileY,
                regionIdValue,
                tier,
                perks,
                askPriceCents,
                rentCents,
                pricing->beds,
                propertyIndex);
            placedInRegion += 1;
        }
    }
}

} // namespace Core
