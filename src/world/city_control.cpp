#include "world/city_control.h"
#include "game/economy_constants.h"

namespace Core {

bool isCityClaimed(const CityControlStore& store, int32_t landmarkIndex) {
    if (landmarkIndex < 0 || landmarkIndex >= getLandmarkCount()) {
        return false;
    }
    return store.slots[landmarkIndex].ownerId != CITY_OWNER_NONE;
}

uint8_t getCityOwnerId(const CityControlStore& store, int32_t landmarkIndex) {
    if (landmarkIndex < 0 || landmarkIndex >= getLandmarkCount()) {
        return CITY_OWNER_NONE;
    }
    return store.slots[landmarkIndex].ownerId;
}

int32_t countPlayerOwnedCities(const CityControlStore& store) {
    int32_t ownedCount = 0;
    const int32_t landmarkCount = getLandmarkCount();
    for (int32_t landmarkIndex = 0; landmarkIndex < landmarkCount; ++landmarkIndex) {
        if (store.slots[landmarkIndex].ownerId == PLAYER_OWNER_ID) {
            ++ownedCount;
        }
    }
    return ownedCount;
}

void resetCityControlStore(CityControlStore& store) {
    for (int32_t landmarkIndex = 0; landmarkIndex < MAX_LANDMARK_COUNT; ++landmarkIndex) {
        store.slots[landmarkIndex] = CityControlSlot{};
    }
}

bool tryClaimCityForPlayer(CityControlStore& store, int32_t landmarkIndex, uint64_t tickCount) {
    if (landmarkIndex < 0 || landmarkIndex >= getLandmarkCount()) {
        return false;
    }
    if (isCityClaimed(store, landmarkIndex)) {
        return false;
    }
    store.slots[landmarkIndex].ownerId = PLAYER_OWNER_ID;
    store.slots[landmarkIndex].claimedTickCount = tickCount;
    return true;
}

} // namespace Core
