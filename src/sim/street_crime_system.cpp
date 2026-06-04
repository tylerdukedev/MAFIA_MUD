#include "sim/street_crime_system.h"
#include "sim/sim_event_queue.h"

namespace Core {

void StreetCrimeSystem::bind(
    const SimWorldBindings& inputBindings,
    PlayerStreetCrimeStore* inputCrimeStore,
    PlayerLawEnforcementStore* inputLawStore,
    PlayerCriminalJusticeStore* inputJusticeStore) {
    bindings = inputBindings;
    crimeStore = inputCrimeStore;
    lawStore = inputLawStore;
    justiceStore = inputJusticeStore;
}

const char* StreetCrimeSystem::getName() const {
    return "StreetCrimeSystem";
}

void StreetCrimeSystem::processCommitStreetCrimeEvent(const SimEvent& event, uint64_t tickCount) {
    if (crimeStore == nullptr || lawStore == nullptr || bindings.playerWallet == nullptr
        || bindings.playerProfile == nullptr || bindings.playerOperationsStore == nullptr
        || bindings.playerOrganizationStore == nullptr || bindings.characterAgentStore == nullptr
        || bindings.worldSeed == nullptr) {
        return;
    }
    const uint8_t regionId = bindings.playerWorldState != nullptr ? bindings.playerWorldState->currentRegionId : static_cast<uint8_t>(1);
    tryCommitStreetCrime(
        *bindings.playerOperationsStore,
        *crimeStore,
        *lawStore,
        *justiceStore,
        *bindings.playerOrganizationStore,
        *bindings.playerWallet,
        *bindings.characterAgentStore,
        *bindings.playerProfile,
        event.catalogIndex,
        tickCount,
        *bindings.worldSeed,
        bindings.playerCriminalRecordStore,
        bindings.playerPoliceContactStore,
        regionId);
}

void StreetCrimeSystem::onTick(uint64_t tickCount) {
    lastTickCount = tickCount;
    if (bindings.eventQueue == nullptr) {
        return;
    }
    SimEvent event{};
    while (popSimEvent(*bindings.eventQueue, event)) {
        if (event.type == SimEventType::CommitStreetCrime) {
            processCommitStreetCrimeEvent(event, tickCount);
            continue;
        }
        pushSimEventRestored(*bindings.eventQueue, event);
    }
}

} // namespace Core
