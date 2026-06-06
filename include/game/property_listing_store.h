#pragma once

#include "core/types.h"
#include "game/property_store.h"
#include <cstdint>

namespace Core {

constexpr int32_t MAX_PROPERTY_LISTINGS = 128;

enum class PropertyListingTier : uint8_t {
    Room = 0,
    Apartment = 1,
    House = 2,
    MultiFamily = 3,
};

enum class PropertyListingPerks : uint8_t {
    None = 0,
    NearTransit = 1U << 0,
    Garage = 1U << 1,
    LowHeat = 1U << 2,
    FamilySpace = 1U << 3,
};

struct PropertyListingRecord {
    int32_t tileX = -1;
    int32_t tileY = -1;
    uint8_t regionId = 0;
    PropertyListingTier tier = PropertyListingTier::Room;
    uint8_t perks = 0;
    int64_t askPriceCents = 0;
    int64_t rentCents = 0;
    uint8_t beds = 1;
    bool onMarket = false;
    int32_t propertyIndex = -1;
};

struct PropertyListingStore {
    PropertyListingRecord records[MAX_PROPERTY_LISTINGS]{};
    int32_t listingCount = 0;
};

void resetPropertyListingStore(PropertyListingStore& store);
int32_t addPropertyListing(
    PropertyListingStore& store,
    int32_t tileX,
    int32_t tileY,
    uint8_t regionId,
    PropertyListingTier tier,
    uint8_t perks,
    int64_t askPriceCents,
    int64_t rentCents,
    uint8_t beds,
    int32_t propertyIndex);
const PropertyListingRecord* getPropertyListingRecord(const PropertyListingStore& store, int32_t listingIndex);
bool isPropertyListingTileOccupied(const PropertyListingStore& store, int32_t tileX, int32_t tileY, int32_t minDistanceTiles);
void syncPropertyListingsToOwnership(PropertyListingStore& listingStore, const PropertyStore& propertyStore);

} // namespace Core
