#pragma once

#include "game/operation_types.h"
#include <cstdint>

namespace Core {

constexpr int32_t MAX_PROPERTY_RECORDS = 128;

enum class PropertyOwnershipType : uint8_t {
    Available = 0,        // Unoccupied, can be rented
    PlayerOwned = 1,      // Player's headquarters
    NpcOwned = 2,         // NPC residence
    BusinessOwned = 3,    // Commercial property
};

struct PropertyRecord {
    int32_t tileX = -1;
    int32_t tileY = -1;
    HeadquartersKind propertyKind = HeadquartersKind::None;
    PropertyOwnershipType ownershipType = PropertyOwnershipType::Available;
    int32_t ownerAgentIndex = -1;  // -1 if player or available, 0+ for NPC agent index
    int32_t monthlyCostCents = 0;
    uint8_t regionId = 0;
};

struct PropertyStore {
    PropertyRecord records[MAX_PROPERTY_RECORDS]{};
    int32_t recordCount = 0;
};

void resetPropertyStore(PropertyStore& store);
int32_t addPropertyRecord(
    PropertyStore& store,
    int32_t tileX,
    int32_t tileY,
    HeadquartersKind kind,
    int32_t monthlyCostCents,
    uint8_t regionId);
bool tryAssignPropertyToNpc(
    PropertyStore& store,
    int32_t propertyIndex,
    int32_t agentIndex);
bool tryAssignPropertyToPlayer(PropertyStore& store, int32_t propertyIndex);
int32_t findAvailableProperty(const PropertyStore& store, HeadquartersKind preferredKind);
const PropertyRecord* getPropertyRecord(const PropertyStore& store, int32_t propertyIndex);
int32_t getNpcPropertyIndex(const PropertyStore& store, int32_t agentIndex);

} // namespace Core
