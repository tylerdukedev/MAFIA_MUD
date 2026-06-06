#pragma once

#include "game/evidence_system.h"
#include "game/investigation_case_store.h"
#include "game/player_law_enforcement.h"
#include "sim/isim_system.h"
#include "sim/sim_world_bindings.h"

namespace Core {

class PoliceSystem : public ISimSystem {
public:
    void bind(
        const SimWorldBindings& inputBindings,
        PlayerLawEnforcementStore* inputLawStore,
        InvestigationCaseStore* inputCaseStore = nullptr,
        EvidenceSystemStore* inputEvidenceStore = nullptr);
    const char* getName() const override;
    void onTick(uint64_t tickCount) override;

private:
    SimWorldBindings bindings{};
    PlayerLawEnforcementStore* lawStore = nullptr;
    InvestigationCaseStore* caseStore = nullptr;
    EvidenceSystemStore* evidenceStore = nullptr;
};

} // namespace Core
