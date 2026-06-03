#pragma once

#include "sim/borough_vitality_system.h"
#include "sim/city_control_system.h"
#include "sim/economy_system.h"
#include "sim/operation_system.h"
#include "sim/criminal_justice_system.h"
#include "sim/police_system.h"
#include "sim/street_crime_system.h"
#include "sim/world_event_system.h"
#include "sim/calendar_system.h"
#include "sim/isim_system.h"
#include "game/game_calendar.h"
#include "game/player_health.h"
#include "game/player_work_schedule.h"
#include "game/player_world_state.h"
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
        PlayerLawEnforcementStore* lawStore,
        PlayerCriminalJusticeStore* justiceStore,
        GameCalendarStore* calendarStore,
        PlayerWorkScheduleStore* workScheduleStore,
        PlayerWorldState* worldState,
        PlayerHealthStore* playerHealthStore,
        PopulationHealthStore* populationHealthStore);
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
    CriminalJusticeSystem criminalJusticeSystem;
    EconomySystem economySystem;
    BoroughVitalitySystem boroughVitalitySystem;
    CalendarSystem calendarSystem;
    ISimSystem* systems[13];
    int32_t systemCount;
};

} // namespace Core
