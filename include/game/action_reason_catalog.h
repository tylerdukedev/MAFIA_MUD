#pragma once

#include "game/agent_relation_events.h"
#include "game/player_law_enforcement.h"
#include "game/player_law_intel.h"
#include "game/player_organization.h"
#include "sim/character_agent.h"
#include <cstdint>

namespace Core {

constexpr int32_t MAX_ACTION_REASON_OFFERS = 12;

enum class ActionReasonId : uint16_t {
    None = 0,
    BribeLearnWarrants = 100,
    BribeLookOtherWay = 101,
    KidnapExtractIntel = 200,
    KidnapRevenge = 201,
    KidnapSendMessage = 202,
    AssassinatePowerPlay = 300,
    AssassinateSilenceWitness = 301,
    AssassinateRevenge = 302,
    AssassinateEliminateRival = 303,
    AssassinateCrewDiscipline = 304,
};

struct ActionReasonDefinition {
    ActionReasonId reasonId;
    CovertActionKind actionKind;
    const char* label;
    const char* description;
    const char* narrativeTag;
    AgentRelationEventFlags requiredFlags;
    AgentRelationEventFlags blockedFlags;
    int32_t heatModifier;
    int32_t minTargetOpinion;
    int32_t maxTargetOpinion;
    bool requiresActiveWarrants;
    bool requiresBeatCopTarget;
    bool requiresRivalSlot;
};

struct ActionReasonOffer {
    ActionReasonId reasonId = ActionReasonId::None;
    char label[48]{};
    char description[96]{};
    char narrativeTag[32]{};
    int32_t heatModifier = 0;
};

int32_t collectCovertActionReasons(
    CovertActionKind actionKind,
    int32_t targetAgentIndex,
    const CharacterAgentStore& agentStore,
    const PlayerOrganizationStore& organizationStore,
    const PlayerLawEnforcementStore& lawStore,
    const PlayerLawIntelStore& intelStore,
    ActionReasonOffer* outOffers,
    int32_t maxOffers);
const ActionReasonDefinition* getActionReasonDefinition(ActionReasonId reasonId);
int32_t resolveCovertActionHeatPenalty(CovertActionKind actionKind, ActionReasonId reasonId, bool usedCrewProxy);
const char* covertActionKindToLabel(CovertActionKind actionKind);

} // namespace Core
