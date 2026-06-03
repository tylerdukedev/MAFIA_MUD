#pragma once

#include "sim/borough_vitality_system.h"
#include "sim/city_control_system.h"
#include "sim/economy_system.h"
#include "sim/operation_system.h"
#include "sim/police_system.h"
#include "sim/street_crime_system.h"
#include "sim/world_event_system.h"
#include "sim/isim_system.h"
#include "sim/sim_world_bindings.h"
#include <cstdint>

namespace Core {

class SystemRegistry {
public:
    SystemRegistry();
    void initialize(
        const SimWorldBindings& bindings,
        CharacterAgentStore* agentStore,
        PlayerStreetCrimeStore* crimeStore,
        PlayerLawEnforcementStore* lawStore);
    void runTick(uint64_t tickCount);
    int32_t getSystemCount() const;
    const ISimSystem* getSystem(int32_t index) const;
    const BoroughVitalitySystem* getBoroughVitalitySystem() const;

private:
    DebugSystem debugSystem;
    StreetCrimeSystem streetCrimeSystem;
    OperationSystem operationSystem;
    CityControlSystem cityControlSystem;
    WorldEventSystem worldEventSystem;
    PoliceSystem policeSystem;
    EconomySystem economySystem;
    BoroughVitalitySystem boroughVitalitySystem;
    ISimSystem* systems[12];
    int32_t systemCount;
};

} // namespace Core
