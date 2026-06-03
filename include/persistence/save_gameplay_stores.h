#pragma once

#include "game/game_calendar.h"
#include "game/legal_counsel.h"
#include "game/player_health.h"
#include "game/player_law_intel.h"
#include "game/player_narrative_archive.h"
#include "game/player_work_schedule.h"
#include "game/player_information_feed.h"
#include "game/player_world_state.h"

namespace Core {

struct SaveGameplayStores {
    GameCalendarStore calendarStore{};
    PlayerWorkScheduleStore workScheduleStore{};
    PlayerLawIntelStore lawIntelStore{};
    PlayerNarrativeArchiveStore narrativeArchiveStore{};
    PlayerHealthStore playerHealthStore{};
    PopulationHealthStore populationHealthStore{};
    PlayerLegalCounselStore legalCounselStore{};
    PlayerWorldState worldState{};
    PlayerInformationFeedStore informationFeedStore{};
};

void resetSaveGameplayStores(SaveGameplayStores& stores);

} // namespace Core
