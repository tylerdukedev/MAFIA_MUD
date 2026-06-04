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
    tickPlayerCriminalJustice(
        *justiceStore,
        *lawStore,
        *bindings.playerWallet,
        *bindings.cityControlStore,
        *bindings.characterAgentStore,
        worldSeed,
        tickCount,
        informationFeedStore);
    tickAgentCriminalJustice(*justiceStore, *bindings.characterAgentStore, *lawStore, worldSeed, tickCount);
}

} // namespace Core
