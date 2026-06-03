#include "sim/borough_vitality_system.h"
#include "world/tile_vitality.h"

namespace Core {

void BoroughVitalitySystem::bind(const SimWorldBindings& inputBindings) {
    bindings = inputBindings;
}

const char* BoroughVitalitySystem::getName() const {
    return "BoroughVitalitySystem";
}

uint64_t BoroughVitalitySystem::getLastRollupTickCount() const {
    return lastRollupTickCount;
}

void BoroughVitalitySystem::onTick(uint64_t tickCount) {
    lastTickCount = tickCount;
    if (bindings.chunkStore == nullptr || bindings.boroughVitalityStore == nullptr || bindings.worldConfig == nullptr) {
        return;
    }
    const uint64_t worldSeed = bindings.worldSeed != nullptr ? *bindings.worldSeed : 0ULL;
    for (int32_t regionIndex = 1; regionIndex < static_cast<int32_t>(RegionId::COUNT); ++regionIndex) {
        tickBoroughPopulation(
            *bindings.boroughVitalityStore,
            static_cast<RegionId>(regionIndex),
            tickCount,
            worldSeed);
    }
    if (tickCount % static_cast<uint64_t>(OPPONENT_LAW_PRESSURE_INTERVAL_TICKS) == 0ULL) {
        tickOpponentPressure(*bindings.chunkStore, tickCount, worldSeed);
    }
    if (tickCount % static_cast<uint64_t>(BOROUGH_ROLLUP_INTERVAL_TICKS) == 0ULL) {
        rollupBoroughVitality(*bindings.worldConfig, *bindings.chunkStore, *bindings.boroughVitalityStore);
        bindings.boroughVitalityStore->lastRollupTickCount = tickCount;
        lastRollupTickCount = tickCount;
        for (int32_t regionIndex = 1; regionIndex < static_cast<int32_t>(RegionId::COUNT); ++regionIndex) {
            distributeBoroughPopulationToTiles(
                *bindings.worldConfig,
                *bindings.chunkStore,
                *bindings.boroughVitalityStore,
                static_cast<RegionId>(regionIndex));
        }
    }
}

} // namespace Core
