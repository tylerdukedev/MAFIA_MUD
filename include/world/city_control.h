#pragma once

#include "game/economy_constants.h"
#include "world/landmark_table.h"
#include <cstdint>

namespace Core {

struct CityControlSlot {
    uint8_t ownerId = CITY_OWNER_NONE;
    uint64_t claimedTickCount = 0;
};

struct CityControlStore {
    CityControlSlot slots[MAX_LANDMARK_COUNT];
};

bool isCityClaimed(const CityControlStore& store, int32_t landmarkIndex);
uint8_t getCityOwnerId(const CityControlStore& store, int32_t landmarkIndex);
int32_t countPlayerOwnedCities(const CityControlStore& store);
void resetCityControlStore(CityControlStore& store);
bool tryClaimCityForPlayer(CityControlStore& store, int32_t landmarkIndex, uint64_t tickCount);

} // namespace Core
