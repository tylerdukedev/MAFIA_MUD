#pragma once

#include "game/evidence_system.h"
#include "game/investigation_case_store.h"
#include "game/street_crime.h"
#include "sim/isim_system.h"
#include "sim/sim_world_bindings.h"

namespace Core {

class StreetCrimeSystem : public ISimSystem {
public:
    void bind(
        const SimWorldBindings& inputBindings,
        PlayerStreetCrimeStore* inputCrimeStore,
        PlayerLawEnforcementStore* inputLawStore,
        PlayerCriminalJusticeStore* inputJusticeStore,
        InvestigationCaseStore* inputCaseStore = nullptr,
        EvidenceSystemStore* inputEvidenceStore = nullptr);
    const char* getName() const override;
    void onTick(uint64_t tickCount) override;

private:
    void processCommitStreetCrimeEvent(const SimEvent& event, uint64_t tickCount);
    SimWorldBindings bindings{};
    PlayerStreetCrimeStore* crimeStore = nullptr;
    PlayerLawEnforcementStore* lawStore = nullptr;
    PlayerCriminalJusticeStore* justiceStore = nullptr;
    InvestigationCaseStore* caseStore = nullptr;
    EvidenceSystemStore* evidenceStore = nullptr;
};

} // namespace Core
