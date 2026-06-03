#include "persistence/save_gameplay_stores.h"

namespace Core {

void resetSaveGameplayStores(SaveGameplayStores& stores) {
    resetGameCalendarStore(stores.calendarStore);
    resetPlayerWorkScheduleStore(stores.workScheduleStore);
    resetPlayerLawIntelStore(stores.lawIntelStore);
    resetPlayerNarrativeArchiveStore(stores.narrativeArchiveStore);
    resetPlayerHealthStore(stores.playerHealthStore);
    resetPopulationHealthStore(stores.populationHealthStore);
    resetPlayerLegalCounselStore(stores.legalCounselStore);
    resetPlayerWorldState(stores.worldState);
    resetPlayerInformationFeedStore(stores.informationFeedStore);
}

} // namespace Core
