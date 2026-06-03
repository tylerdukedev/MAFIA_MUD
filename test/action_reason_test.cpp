#include "game/action_reason_catalog.h"
#include "game/agent_relation_events.h"
#include "sim/character_agent.h"
#include "character/character_social_network.h"
#include <catch2/catch_test_macros.hpp>

using namespace Core;

TEST_CASE("Assassinate revenge reason unlocks after betrayal flag", "[action_reason]") {
    CharacterAgentStore agentStore{};
    PlayerOrganizationStore organizationStore{};
    PlayerLawEnforcementStore lawStore{};
    PlayerLawIntelStore intelStore{};
    CharacterAgentState& rivalState = agentStore.states[RIVAL_AGENT_SLOT_INDEX];
    rivalState.isActive = true;
    rivalState.opinionOfPlayer = -40;
    setAgentRelationEvent(rivalState, AgentRelationEventFlags::BetrayedPlayer);
    ActionReasonOffer offers[MAX_ACTION_REASON_OFFERS]{};
    const int32_t offerCount = collectCovertActionReasons(
        CovertActionKind::AssassinateTarget,
        RIVAL_AGENT_SLOT_INDEX,
        agentStore,
        organizationStore,
        lawStore,
        intelStore,
        offers,
        MAX_ACTION_REASON_OFFERS);
    bool hasRevenge = false;
    for (int32_t offerIndex = 0; offerIndex < offerCount; ++offerIndex) {
        if (offers[offerIndex].reasonId == ActionReasonId::AssassinateRevenge) {
            hasRevenge = true;
        }
    }
    REQUIRE(hasRevenge);
}

TEST_CASE("Bribe warrant reason requires active warrants", "[action_reason]") {
    CharacterAgentStore agentStore{};
    agentStore.states[BEAT_COP_AGENT_SLOT_INDEX].isActive = true;
    PlayerLawEnforcementStore lawStore{};
    lawStore.activeWarrantCount = 0;
    ActionReasonOffer offers[MAX_ACTION_REASON_OFFERS]{};
    const int32_t offerCount = collectCovertActionReasons(
        CovertActionKind::BribePolice,
        BEAT_COP_AGENT_SLOT_INDEX,
        agentStore,
        PlayerOrganizationStore{},
        lawStore,
        PlayerLawIntelStore{},
        offers,
        MAX_ACTION_REASON_OFFERS);
    bool hasWarrantReason = false;
    for (int32_t offerIndex = 0; offerIndex < offerCount; ++offerIndex) {
        if (offers[offerIndex].reasonId == ActionReasonId::BribeLearnWarrants) {
            hasWarrantReason = true;
        }
    }
    REQUIRE_FALSE(hasWarrantReason);
    lawStore.activeWarrantCount = 2;
    const int32_t offerCountAfter = collectCovertActionReasons(
        CovertActionKind::BribePolice,
        BEAT_COP_AGENT_SLOT_INDEX,
        agentStore,
        PlayerOrganizationStore{},
        lawStore,
        PlayerLawIntelStore{},
        offers,
        MAX_ACTION_REASON_OFFERS);
    REQUIRE(offerCountAfter >= 1);
}
