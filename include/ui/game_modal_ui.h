#pragma once

#include "character/player_profile.h"
#include "core/sim_clock.h"
#include "game/game_calendar.h"
#include "game/player_criminal_justice.h"
#include "game/player_health.h"
#include "game/player_law_enforcement.h"
#include "game/legal_counsel.h"
#include "game/player_organization.h"
#include "game/player_operations.h"
#include "game/player_wallet.h"
#include "game/player_work_schedule.h"
#include "game/player_law_intel.h"
#include "game/player_information_feed.h"
#include "game/player_narrative_archive.h"
#include "game/action_reason_catalog.h"
#include "game/player_world_state.h"
#include "sim/character_agent.h"
#include "sim/sim_event_queue.h"
#include "ui/game_modal_state.h"
#include <cstdint>

namespace Core {

void beginJobInterviewModal(GameModalState& modal, int32_t businessNodeIndex, SimClock& simClock, uint64_t worldSeed);
void beginApartmentApplicationModal(GameModalState& modal, int32_t catalogIndex, SimClock& simClock);
void beginWorkDayCommuteModal(GameModalState& modal, bool isLateForWork, SimClock& simClock);
void beginCrewRecruitmentModal(GameModalState& modal, int32_t agentIndex, SimClock& simClock);
void beginCrewFormalizeModal(GameModalState& modal, SimClock& simClock);
void beginOrganizationCreationModal(GameModalState& modal, SimClock& simClock);
void beginBondHearingModal(GameModalState& modal, SimClock& simClock);
void beginCourtHearingModal(GameModalState& modal, SimClock& simClock);
void beginInformationFeedModal(GameModalState& modal, int32_t feedItemIndex, SimClock& simClock);
void beginCovertActionModal(
    GameModalState& modal,
    CovertActionKind actionKind,
    int32_t targetAgentIndex,
    const CharacterAgentStore& agentStore,
    const PlayerOrganizationStore& organizationStore,
    const PlayerLawEnforcementStore& lawStore,
    const PlayerLawIntelStore& intelStore,
    SimClock& simClock);
void tickCriminalJusticeModals(
    GameModalState& modal,
    PlayerCriminalJusticeStore& justiceStore,
    SimClock& simClock);
void tickWorkScheduleModals(
    GameModalState& modal,
    const PlayerWorkScheduleStore& workScheduleStore,
    const GameCalendarStore& calendarStore,
    SimClock& simClock);
void renderGameModalOverlay(
    GameModalState& modal,
    SimClock& simClock,
    PlayerOperationsStore& playerOperationsStore,
    PlayerOrganizationStore& playerOrganizationStore,
    PlayerLawEnforcementStore& playerLawEnforcementStore,
    PlayerCriminalJusticeStore& playerCriminalJusticeStore,
    PlayerLegalCounselStore& legalCounselStore,
    PlayerHealthStore& playerHealthStore,
    PlayerLawIntelStore& lawIntelStore,
    PlayerNarrativeArchiveStore& narrativeArchiveStore,
    PlayerInformationFeedStore& informationFeedStore,
    PlayerWallet& playerWallet,
    PlayerWorldState& playerWorldState,
    const ChunkStore& chunkStore,
    const WorldConfig& worldConfig,
    PlayerWorkScheduleStore& workScheduleStore,
    GameCalendarStore& calendarStore,
    CharacterAgentStore& characterAgentStore,
    SimEventQueue& simEventQueue,
    const PlayerProfile& playerProfile,
    uint64_t tickCount,
    uint64_t worldSeed);

} // namespace Core
