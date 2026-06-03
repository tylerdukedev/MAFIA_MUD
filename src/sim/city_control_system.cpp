#include "sim/city_control_system.h"
#include "sim/sim_event_queue.h"
#include "game/player_operations.h"
#include "game/player_wallet.h"
#include "world/landmark_table.h"
#include "world/tile_vitality.h"
#include <algorithm>

namespace Core {

namespace {
int64_t computeClaimCostCents(const PlayerProfile* profile) {
    if (profile == nullptr) {
        return DEFAULT_CLAIM_CITY_COST_CENTS;
    }
    if (profile->draft.backgroundId == BackgroundId::StreetHustler) {
        return STREET_HUSTLER_CLAIM_COST_CENTS;
    }
    return DEFAULT_CLAIM_CITY_COST_CENTS;
}

void boostPlayerInfluenceAtLandmark(ChunkStore& chunkStore, const LandmarkDefinition& landmark) {
    const int32_t radius = static_cast<int32_t>(landmark.heatRadiusTiles);
    for (int32_t offsetY = -radius; offsetY <= radius; ++offsetY) {
        for (int32_t offsetX = -radius; offsetX <= radius; ++offsetX) {
            const WorldCoord coord{landmark.tileX + offsetX, landmark.tileY + offsetY};
            if (chunkStore.getTerrainAt(coord) == TerrainId::Water) {
                continue;
            }
            const uint8_t influence = static_cast<uint8_t>(std::min(255, static_cast<int32_t>(chunkStore.getPlayerInfluenceAt(coord)) + 48));
            chunkStore.setPlayerInfluenceAt(coord, influence);
        }
    }
}
} // namespace

void CityControlSystem::bind(const SimWorldBindings& inputBindings) {
    bindings = inputBindings;
}

const char* CityControlSystem::getName() const {
    return "CityControlSystem";
}

void CityControlSystem::processClaimCityEvent(const SimEvent& event, uint64_t tickCount) {
    if (bindings.chunkStore == nullptr || bindings.cityControlStore == nullptr || bindings.playerWallet == nullptr) {
        return;
    }
    const int32_t landmarkIndex = event.landmarkIndex;
    const LandmarkDefinition* landmark = getLandmarkDefinition(landmarkIndex);
    if (landmark == nullptr) {
        return;
    }
    if (isCityClaimed(*bindings.cityControlStore, landmarkIndex)) {
        return;
    }
    if (bindings.playerOperationsStore == nullptr || bindings.playerProfile == nullptr) {
        return;
    }
    int64_t claimCostCents = 0;
    if (!canEstablishCityOperation(*bindings.playerOperationsStore, *bindings.playerProfile, *bindings.playerWallet, claimCostCents)) {
        return;
    }
    if (!tryDebitCash(*bindings.playerWallet, claimCostCents)) {
        return;
    }
    if (!tryClaimCityForPlayer(*bindings.cityControlStore, landmarkIndex, tickCount)) {
        restoreCashWithoutDelta(*bindings.playerWallet, claimCostCents);
        return;
    }
    boostPlayerInfluenceAtLandmark(*bindings.chunkStore, *landmark);
}

void CityControlSystem::onTick(uint64_t tickCount) {
    lastTickCount = tickCount;
    if (bindings.eventQueue == nullptr) {
        return;
    }
    SimEvent event{};
    while (popSimEvent(*bindings.eventQueue, event)) {
        if (event.type == SimEventType::ClaimCity) {
            processClaimCityEvent(event, tickCount);
            continue;
        }
        pushSimEventRestored(*bindings.eventQueue, event);
    }
}

} // namespace Core
