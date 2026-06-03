#include "game/action_reason_catalog.h"
#include "character/character_social_network.h"
#include <cstdio>
#include <cstring>

namespace Core {

namespace {

constexpr ActionReasonDefinition ACTION_REASON_TABLE[] = {
    {ActionReasonId::BribeLearnWarrants, CovertActionKind::BribePolice, "Learn about warrants", "Pay for warrant details the desk has not filed yet.", "bribe_warrants", AgentRelationEventFlags::None, AgentRelationEventFlags::None, 0, -100, 100, true, true, false},
    {ActionReasonId::BribeLookOtherWay, CovertActionKind::BribePolice, "Look the other way", "Grease a patrolman before your name spreads.", "bribe_delay", AgentRelationEventFlags::None, AgentRelationEventFlags::None, 4, -100, 100, false, true, false},
    {ActionReasonId::KidnapExtractIntel, CovertActionKind::KidnapTarget, "Extract intel", "Rough interrogation for names, warrants, and weak points.", "kidnap_intel", AgentRelationEventFlags::None, AgentRelationEventFlags::None, 0, -100, 80, false, false, false},
    {ActionReasonId::KidnapRevenge, CovertActionKind::KidnapTarget, "Revenge", "Settle a personal score — they will remember it.", "kidnap_revenge", AgentRelationEventFlags::BetrayedPlayer, AgentRelationEventFlags::None, 6, -100, 40, false, false, false},
    {ActionReasonId::KidnapSendMessage, CovertActionKind::KidnapTarget, "Send a message", "Let the neighborhood see what happens to loud mouths.", "kidnap_message", AgentRelationEventFlags::InsultedPlayer, AgentRelationEventFlags::None, 4, -100, 100, false, false, false},
    {ActionReasonId::AssassinatePowerPlay, CovertActionKind::AssassinateTarget, "Power play", "Remove a problem and show the street who runs things.", "kill_power", AgentRelationEventFlags::None, AgentRelationEventFlags::None, 8, -100, 100, false, false, false},
    {ActionReasonId::AssassinateSilenceWitness, CovertActionKind::AssassinateTarget, "Silence a witness", "Stop testimony before it reaches a grand jury.", "kill_witness", AgentRelationEventFlags::SnitchedToPolice, AgentRelationEventFlags::None, -4, -100, 100, false, false, false},
    {ActionReasonId::AssassinateRevenge, CovertActionKind::AssassinateTarget, "Revenge", "They crossed you — this is personal.", "kill_revenge", AgentRelationEventFlags::BetrayedPlayer, AgentRelationEventFlags::None, -6, -100, 60, false, false, false},
    {ActionReasonId::AssassinateEliminateRival, CovertActionKind::AssassinateTarget, "Eliminate rival", "Your standing rival stops being a problem tonight.", "kill_rival", AgentRelationEventFlags::MarkedRival, AgentRelationEventFlags::None, 2, -100, 100, false, false, true},
    {ActionReasonId::AssassinateCrewDiscipline, CovertActionKind::AssassinateTarget, "Crew discipline", "An example for anyone thinking of going independent.", "kill_discipline", AgentRelationEventFlags::CrewDefector, AgentRelationEventFlags::None, 0, -100, 100, false, false, false},
};

bool passesRelationGate(const CharacterAgentState& state, const ActionReasonDefinition& definition) {
    if (definition.requiredFlags != AgentRelationEventFlags::None) {
        const uint16_t required = static_cast<uint16_t>(definition.requiredFlags);
        if ((state.relationEventFlags & required) != required) {
            return false;
        }
    }
    if (definition.blockedFlags != AgentRelationEventFlags::None) {
        const uint16_t blocked = static_cast<uint16_t>(definition.blockedFlags);
        if ((state.relationEventFlags & blocked) != 0U) {
            return false;
        }
    }
    return true;
}

bool passesTargetGate(
    const ActionReasonDefinition& definition,
    int32_t targetAgentIndex,
    const CharacterAgentState& state,
    const PlayerLawEnforcementStore& lawStore) {
    if (state.opinionOfPlayer < definition.minTargetOpinion || state.opinionOfPlayer > definition.maxTargetOpinion) {
        return false;
    }
    if (definition.requiresActiveWarrants && lawStore.activeWarrantCount <= 0) {
        return false;
    }
    if (definition.requiresBeatCopTarget && targetAgentIndex != BEAT_COP_AGENT_SLOT_INDEX) {
        return false;
    }
    if (definition.requiresRivalSlot && targetAgentIndex != RIVAL_AGENT_SLOT_INDEX) {
        return false;
    }
    return true;
}

void copyOfferFromDefinition(ActionReasonOffer& offer, const ActionReasonDefinition& definition) {
    offer.reasonId = definition.reasonId;
    std::snprintf(offer.label, sizeof(offer.label), "%s", definition.label);
    std::snprintf(offer.description, sizeof(offer.description), "%s", definition.description);
    std::snprintf(offer.narrativeTag, sizeof(offer.narrativeTag), "%s", definition.narrativeTag);
    offer.heatModifier = definition.heatModifier;
}

} // namespace

const ActionReasonDefinition* getActionReasonDefinition(ActionReasonId reasonId) {
    for (const ActionReasonDefinition& definition : ACTION_REASON_TABLE) {
        if (definition.reasonId == reasonId) {
            return &definition;
        }
    }
    return nullptr;
}

const char* covertActionKindToLabel(CovertActionKind actionKind) {
    switch (actionKind) {
    case CovertActionKind::BribePolice:
        return "Bribe police";
    case CovertActionKind::KidnapTarget:
        return "Kidnap";
    case CovertActionKind::AssassinateTarget:
        return "Assassinate";
    default:
        return "Action";
    }
}

int32_t collectCovertActionReasons(
    CovertActionKind actionKind,
    int32_t targetAgentIndex,
    const CharacterAgentStore& agentStore,
    const PlayerOrganizationStore& organizationStore,
    const PlayerLawEnforcementStore& lawStore,
    const PlayerLawIntelStore& intelStore,
    ActionReasonOffer* outOffers,
    int32_t maxOffers) {
    (void)organizationStore;
    (void)intelStore;
    if (outOffers == nullptr || maxOffers <= 0) {
        return 0;
    }
    const CharacterAgentState* targetState = getCharacterAgentState(agentStore, targetAgentIndex);
    if (targetState == nullptr) {
        return 0;
    }
    int32_t offerCount = 0;
    for (const ActionReasonDefinition& definition : ACTION_REASON_TABLE) {
        if (definition.actionKind != actionKind) {
            continue;
        }
        if (!passesRelationGate(*targetState, definition)) {
            continue;
        }
        if (!passesTargetGate(definition, targetAgentIndex, *targetState, lawStore)) {
            continue;
        }
        copyOfferFromDefinition(outOffers[offerCount], definition);
        offerCount += 1;
        if (offerCount >= maxOffers) {
            break;
        }
    }
    return offerCount;
}

int32_t resolveCovertActionHeatPenalty(CovertActionKind actionKind, ActionReasonId reasonId, bool usedCrewProxy) {
    int32_t baseHeat = KIDNAP_HEAT_PENALTY;
    if (actionKind == CovertActionKind::AssassinateTarget) {
        baseHeat = ASSASSINATE_HEAT_PENALTY;
    } else if (actionKind == CovertActionKind::BribePolice) {
        baseHeat = 6;
    }
    const ActionReasonDefinition* definition = getActionReasonDefinition(reasonId);
    if (definition != nullptr) {
        baseHeat += definition->heatModifier;
    }
    if (usedCrewProxy && actionKind == CovertActionKind::AssassinateTarget) {
        baseHeat = baseHeat / 2 + 8;
    }
    if (baseHeat < 2) {
        baseHeat = 2;
    }
    return baseHeat;
}

} // namespace Core
