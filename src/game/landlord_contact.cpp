#include "game/landlord_contact.h"
#include "character/character_social_network.h"

namespace Core {

void spawnLandlordContact(CharacterAgentStore& agentStore) {
    const int32_t landlordSlot = FIRST_COMMUNITY_AGENT_SLOT_INDEX;
    CharacterAgentState& state = agentStore.states[landlordSlot];
    state.hasGeneratedIdentity = false;
    state.isActive = true;
    state.opinionOfPlayer = -8;
    deriveRelationshipStatsFromOpinion(state);
    state.currentEmotion = AgentEmotion::Suspicious;
}

} // namespace Core
