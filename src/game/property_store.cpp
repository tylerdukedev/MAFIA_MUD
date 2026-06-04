#include "game/property_store.h"
#include <cstring>

namespace Core {

void resetPropertyStore(PropertyStore& store) {
    for (int32_t i = 0; i < MAX_PROPERTY_RECORDS; ++i) {
        store.records[i] = PropertyRecord{};
    }
    store.recordCount = 0;
}

int32_t addPropertyRecord(
    PropertyStore& store,
    int32_t tileX,
    int32_t tileY,
    HeadquartersKind kind,
    int32_t monthlyCostCents,
    uint8_t regionId) {
    if (store.recordCount >= MAX_PROPERTY_RECORDS) {
        return -1;
    }
    const int32_t index = store.recordCount;
    PropertyRecord& record = store.records[index];
    record.tileX = tileX;
    record.tileY = tileY;
    record.propertyKind = kind;
    record.ownershipType = PropertyOwnershipType::Available;
    record.ownerAgentIndex = -1;
    record.monthlyCostCents = monthlyCostCents;
    record.regionId = regionId;
    store.recordCount += 1;
    return index;
}

bool tryAssignPropertyToNpc(
    PropertyStore& store,
    int32_t propertyIndex,
    int32_t agentIndex) {
    if (propertyIndex < 0 || propertyIndex >= store.recordCount) {
        return false;
    }
    PropertyRecord& record = store.records[propertyIndex];
    if (record.ownershipType != PropertyOwnershipType::Available) {
        return false;
    }
    record.ownershipType = PropertyOwnershipType::NpcOwned;
    record.ownerAgentIndex = agentIndex;
    return true;
}

bool tryAssignPropertyToPlayer(PropertyStore& store, int32_t propertyIndex) {
    if (propertyIndex < 0 || propertyIndex >= store.recordCount) {
        return false;
    }
    PropertyRecord& record = store.records[propertyIndex];
    if (record.ownershipType != PropertyOwnershipType::Available) {
        return false;
    }
    record.ownershipType = PropertyOwnershipType::PlayerOwned;
    record.ownerAgentIndex = -1;
    return true;
}

int32_t findAvailableProperty(const PropertyStore& store, HeadquartersKind preferredKind) {
    for (int32_t i = 0; i < store.recordCount; ++i) {
        const PropertyRecord& record = store.records[i];
        if (record.ownershipType == PropertyOwnershipType::Available
            && record.propertyKind == preferredKind) {
            return i;
        }
    }
    return -1;
}

const PropertyRecord* getPropertyRecord(const PropertyStore& store, int32_t propertyIndex) {
    if (propertyIndex < 0 || propertyIndex >= store.recordCount) {
        return nullptr;
    }
    return &store.records[propertyIndex];
}

int32_t getNpcPropertyIndex(const PropertyStore& store, int32_t agentIndex) {
    for (int32_t i = 0; i < store.recordCount; ++i) {
        const PropertyRecord& record = store.records[i];
        if (record.ownershipType == PropertyOwnershipType::NpcOwned
            && record.ownerAgentIndex == agentIndex) {
            return i;
        }
    }
    return -1;
}

} // namespace Core
