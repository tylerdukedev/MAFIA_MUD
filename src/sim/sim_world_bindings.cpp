#include "sim/sim_world_bindings.h"

namespace Core {

bool isSimWorldBindingsValid(const SimWorldBindings& bindings) {
    return bindings.chunkStore != nullptr
        && bindings.boroughVitalityStore != nullptr
        && bindings.worldConfig != nullptr
        && bindings.worldSeed != nullptr
        && bindings.playerWallet != nullptr
        && bindings.cityControlStore != nullptr
        && bindings.eventQueue != nullptr
        && bindings.playerProfile != nullptr
        && bindings.playerOperationsStore != nullptr
        && bindings.playerOrganizationStore != nullptr
        && bindings.playerLawEnforcementStore != nullptr
        && bindings.playerCriminalJusticeStore != nullptr
        && bindings.playerStreetCrimeStore != nullptr
        && bindings.characterAgentStore != nullptr
        && bindings.worldEventStore != nullptr;
}

} // namespace Core
