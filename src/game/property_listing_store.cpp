#include "game/property_listing_store.h"
#include <cmath>

namespace Core {

void resetPropertyListingStore(PropertyListingStore& store) {
    for (int32_t index = 0; index < MAX_PROPERTY_LISTINGS; ++index) {
        store.records[index] = PropertyListingRecord{};
    }
    store.listingCount = 0;
}

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
    int32_t propertyIndex) {
    if (store.listingCount >= MAX_PROPERTY_LISTINGS) {
        return -1;
    }
    const int32_t index = store.listingCount;
    PropertyListingRecord& record = store.records[index];
    record.tileX = tileX;
    record.tileY = tileY;
    record.regionId = regionId;
    record.tier = tier;
    record.perks = perks;
    record.askPriceCents = askPriceCents;
    record.rentCents = rentCents;
    record.beds = beds;
    record.onMarket = true;
    record.propertyIndex = propertyIndex;
    store.listingCount += 1;
    return index;
}

const PropertyListingRecord* getPropertyListingRecord(const PropertyListingStore& store, int32_t listingIndex) {
    if (listingIndex < 0 || listingIndex >= store.listingCount) {
        return nullptr;
    }
    return &store.records[listingIndex];
}

void syncPropertyListingsToOwnership(PropertyListingStore& listingStore, const PropertyStore& propertyStore) {
    for (int32_t index = 0; index < listingStore.listingCount; ++index) {
        PropertyListingRecord& listing = listingStore.records[index];
        if (listing.propertyIndex < 0) {
            listing.onMarket = false;
            continue;
        }
        const PropertyRecord* record = getPropertyRecord(propertyStore, listing.propertyIndex);
        listing.onMarket = record != nullptr && record->ownershipType == PropertyOwnershipType::Available;
    }
}

bool isPropertyListingTileOccupied(
    const PropertyListingStore& store,
    int32_t tileX,
    int32_t tileY,
    int32_t minDistanceTiles) {
    const int32_t minDistanceSquared = minDistanceTiles * minDistanceTiles;
    for (int32_t index = 0; index < store.listingCount; ++index) {
        const PropertyListingRecord& record = store.records[index];
        const int32_t deltaX = record.tileX - tileX;
        const int32_t deltaY = record.tileY - tileY;
        const int32_t distanceSquared = deltaX * deltaX + deltaY * deltaY;
        if (distanceSquared <= minDistanceSquared) {
            return true;
        }
    }
    return false;
}

} // namespace Core
