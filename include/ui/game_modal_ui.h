#pragma once

#include "character/player_profile.h"
#include "core/sim_clock.h"
#include "game/player_law_enforcement.h"
#include "game/player_organization.h"
#include "game/player_operations.h"
#include "game/player_wallet.h"
#include "game/player_world_state.h"
#include "sim/character_agent.h"
#include "sim/sim_event_queue.h"
#include "ui/game_modal_state.h"
#include <cstdint>

namespace Core {

void beginJobInterviewModal(GameModalState& modal, int32_t businessNodeIndex, SimClock& simClock);
void beginApartmentApplicationModal(GameModalState& modal, int32_t catalogIndex, SimClock& simClock);
void beginWorkDayCommuteModal(GameModalState& modal, bool isLateForWork, SimClock& simClock);
void beginCrewRecruitmentModal(GameModalState& modal, int32_t agentIndex, SimClock& simClock);
void beginCrewFormalizeModal(GameModalState& modal, SimClock& simClock);
void beginOrganizationCreationModal(GameModalState& modal, SimClock& simClock);
void renderGameModalOverlay(
    GameModalState& modal,
    SimClock& simClock,
    PlayerOperationsStore& playerOperationsStore,
    PlayerOrganizationStore& playerOrganizationStore,
    PlayerLawEnforcementStore& playerLawEnforcementStore,
    PlayerWallet& playerWallet,
    PlayerWorldState& playerWorldState,
    CharacterAgentStore& characterAgentStore,
    SimEventQueue& simEventQueue,
    const PlayerProfile& playerProfile,
    uint64_t tickCount,
    uint64_t worldSeed);

void tickWorkDayCommutePrompt(
    GameModalState& modal,
    PlayerWorldState& playerWorldState,
    const PlayerOperationsStore& playerOperationsStore,
    SimClock& simClock,
    uint64_t tickCount);

} // namespace Core
