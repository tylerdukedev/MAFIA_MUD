#include "game/housing_economy.h"
#include "sim/world_event_types.h"
#include <algorithm>

namespace Core {

namespace {

constexpr int32_t RENT_MULTIPLIER_MIN_BPS = 8200;
constexpr int32_t RENT_MULTIPLIER_MAX_BPS = 12800;
constexpr int32_t PLAYER_INFLUENCE_RENT_RELIEF_MAX_BPS = 900;

} // namespace

int32_t computeHousingRentMultiplierBps(
    const BoroughVitalityStore& boroughVitalityStore,
    const ChunkStore& chunkStore,
    uint8_t headquartersRegionId) {
    (void)chunkStore;
    const RegionId regionId = static_cast<RegionId>(headquartersRegionId);
    const BoroughVitalitySnapshot* snapshot = getBoroughSnapshot(boroughVitalityStore, regionId);
    if (snapshot == nullptr) {
        return RENT_MULTIPLIER_BPS_BASE;
    }
    const float healthNormalized = std::clamp(snapshot->economicHealth / 100.0f, 0.0f, 1.0f);
    int32_t multiplierBps = RENT_MULTIPLIER_MIN_BPS + static_cast<int32_t>((RENT_MULTIPLIER_MAX_BPS - RENT_MULTIPLIER_MIN_BPS) * healthNormalized);
    const float influenceNormalized = std::clamp(snapshot->playerInfluenceSum / 4000.0f, 0.0f, 1.0f);
    const int32_t reliefBps = static_cast<int32_t>(influenceNormalized * static_cast<float>(PLAYER_INFLUENCE_RENT_RELIEF_MAX_BPS));
    multiplierBps -= reliefBps;
    return std::clamp(multiplierBps, RENT_MULTIPLIER_MIN_BPS, RENT_MULTIPLIER_MAX_BPS);
}

void updatePlayerHousingRentMultiplier(
    PlayerOperationsStore& operationsStore,
    const BoroughVitalityStore& boroughVitalityStore,
    const ChunkStore& chunkStore) {
    if (!hasPlayerHeadquarters(operationsStore)) {
        operationsStore.rentMultiplierBps = RENT_MULTIPLIER_BPS_BASE;
        return;
    }
    if (operationsStore.headquartersRegionId == 0U) {
        operationsStore.rentMultiplierBps = RENT_MULTIPLIER_BPS_BASE;
        return;
    }
    operationsStore.rentMultiplierBps = computeHousingRentMultiplierBps(
        boroughVitalityStore,
        chunkStore,
        operationsStore.headquartersRegionId);
}

} // namespace Core
