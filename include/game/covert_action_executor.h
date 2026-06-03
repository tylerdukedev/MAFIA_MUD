#pragma once

#include "game/action_reason_catalog.h"
#include "game/game_calendar.h"
#include "game/player_law_enforcement.h"
#include "game/player_law_intel.h"
#include "game/player_narrative_archive.h"
#include "game/player_organization.h"
#include "game/player_wallet.h"
#include "sim/character_agent.h"
#include <cstdint>

namespace Core {

struct CovertActionResult {
    bool succeeded = false;
    int32_t heatApplied = 0;
};

CovertActionResult executeCovertActionWithReason(
    CovertActionKind actionKind,
    ActionReasonId reasonId,
    int32_t targetAgentIndex,
    bool usedCrewProxy,
    PlayerLawIntelStore& intelStore,
    PlayerLawEnforcementStore& lawStore,
    CharacterAgentStore& agentStore,
    PlayerWallet& wallet,
    PlayerOrganizationStore& organizationStore,
    PlayerNarrativeArchiveStore& narrativeStore,
    const GameCalendarStore& calendarStore,
    uint64_t worldSeed,
    uint64_t tickCount);

} // namespace Core
