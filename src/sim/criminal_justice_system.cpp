#include "sim/criminal_justice_system.h"

namespace Core {

void CriminalJusticeSystem::bind(
    const SimWorldBindings& inputBindings,
    PlayerCriminalJusticeStore* inputJusticeStore,
    PlayerLawEnforcementStore* inputLawStore,
    PlayerInformationFeedStore* inputInformationFeedStore) {
    bindings = inputBindings;
    justiceStore = inputJusticeStore;
    lawStore = inputLawStore;
    informationFeedStore = inputInformationFeedStore;
}

const char* CriminalJusticeSystem::getName() const {
    return "CriminalJusticeSystem";
}

void CriminalJusticeSystem::onTick(uint64_t tickCount) {
    lastTickCount = tickCount;
    if (justiceStore == nullptr || lawStore == nullptr || bindings.cityControlStore == nullptr
        || bindings.characterAgentStore == nullptr || bindings.playerWallet == nullptr || bindings.worldSeed == nullptr) {
        return;
    }
    const uint64_t worldSeed = *bindings.worldSeed;
    const uint8_t regionId = bindings.playerWorldState != nullptr ? bindings.playerWorldState->currentRegionId : static_cast<uint8_t>(1);
    tickPlayerCriminalJustice(
        *justiceStore,
        *lawStore,
        *bindings.playerWallet,
        *bindings.cityControlStore,
        *bindings.characterAgentStore,
        worldSeed,
        tickCount,
        informationFeedStore,
        bindings.playerCriminalRecordStore,
        bindings.playerPoliceContactStore,
        regionId);
    tickAgentCriminalJustice(*justiceStore, *bindings.characterAgentStore, *lawStore, worldSeed, tickCount);
}

void CriminalJusticeSystem::updateFrame(double deltaSeconds, const SimClock& simClock) {
    if (justiceStore == nullptr) {
        return;
    }
    tickPlayerCustodyTimers(*justiceStore, deltaSeconds, simClock.isPaused(), simClock.getTickCount());
}

} // namespace Core
