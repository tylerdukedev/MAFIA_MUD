#include "game/covert_action_executor.h"
#include "character/character_social_network.h"

namespace Core {

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
    uint64_t tickCount) {
    (void)organizationStore;
    CovertActionResult result{};
    const char* displayName = nullptr;
    const char* roleLabel = nullptr;
    tryGetAgentDisplayLabels(agentStore, targetAgentIndex, displayName, roleLabel);
    if (actionKind == CovertActionKind::BribePolice) {
        result.succeeded = tryBribePoliceOfficer(intelStore, lawStore, agentStore, wallet, targetAgentIndex);
        result.heatApplied = result.succeeded ? 4 : 8;
    } else if (actionKind == CovertActionKind::KidnapTarget) {
        result.heatApplied = resolveCovertActionHeatPenalty(actionKind, reasonId, false);
        result.succeeded = tryKidnapForIntel(intelStore, lawStore, agentStore, targetAgentIndex, worldSeed, tickCount);
    } else if (actionKind == CovertActionKind::AssassinateTarget) {
        result.heatApplied = resolveCovertActionHeatPenalty(actionKind, reasonId, usedCrewProxy);
        result.succeeded = tryAssassinateTarget(
            intelStore,
            lawStore,
            agentStore,
            targetAgentIndex,
            usedCrewProxy,
            result.heatApplied,
            worldSeed,
            tickCount);
    }
    appendNarrativeBeat(
        narrativeStore,
        actionKind,
        reasonId,
        targetAgentIndex,
        calendarStore.totalDaysElapsed,
        tickCount,
        displayName);
    return result;
}

} // namespace Core
