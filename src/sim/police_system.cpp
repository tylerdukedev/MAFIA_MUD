#include "sim/police_system.h"
#include <algorithm>

namespace Core {

void PoliceSystem::bind(const SimWorldBindings& inputBindings, PlayerLawEnforcementStore* inputLawStore) {
    bindings = inputBindings;
    lawStore = inputLawStore;
}

const char* PoliceSystem::getName() const {
    return "PoliceSystem";
}

void PoliceSystem::onTick(uint64_t tickCount) {
    lastTickCount = tickCount;
    if (lawStore == nullptr || bindings.playerProfile == nullptr) {
        return;
    }
    tickPoliceEvidenceDecay(*lawStore, tickCount);
    tickPoliceWarrantReview(*lawStore, tickCount);
    if (lawStore->personalHeat <= 0) {
        return;
    }
    if (lawStore->lastHeatDecayTick != 0ULL
        && tickCount - lawStore->lastHeatDecayTick < static_cast<uint64_t>(POLICE_HEAT_DECAY_INTERVAL_TICKS)) {
        return;
    }
    lawStore->lastHeatDecayTick = tickCount;
    const float decayScale = 1.0f + bindings.playerProfile->legitimacy.policeAttentionDecay;
    const int32_t decayAmount = std::max(1, static_cast<int32_t>(decayScale));
    lawStore->personalHeat = std::max(PLAYER_HEAT_MIN, lawStore->personalHeat - decayAmount);
    refreshInvestigationTier(*lawStore);
}

} // namespace Core
