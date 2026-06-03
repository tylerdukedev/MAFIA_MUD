#include "game/kin_housing.h"
#include "character/character_social_network.h"
#include "game/operation_types.h"
#include "game/player_operations.h"
#include "sim/character_agent.h"
#include <algorithm>
#include <cstdio>
#include <cstring>

namespace Core {

void activateKinLandlordFromDpa(const PlayerProfile& profile, CharacterAgentStore& agentStore) {
    int32_t kinSlotIndex = FRIEND_AGENT_SLOT_INDEX;
    if (profile.draft.hasFamilyInCountry && agentStore.states[FAMILY_AGENT_SLOT_INDEX].isActive) {
        kinSlotIndex = FAMILY_AGENT_SLOT_INDEX;
    } else if (!agentStore.states[FRIEND_AGENT_SLOT_INDEX].isActive) {
        return;
    }
    const CharacterAgentState& kinState = agentStore.states[kinSlotIndex];
    CharacterAgentState& landlordState = agentStore.states[FIRST_COMMUNITY_AGENT_SLOT_INDEX];
    landlordState = kinState;
    landlordState.isActive = true;
    std::snprintf(
        landlordState.generatedRoleLabel,
        sizeof(landlordState.generatedRoleLabel),
        "Landlord (%s)",
        kinState.generatedRoleLabel);
    landlordState.opinionOfPlayer = std::min(landlordState.opinionOfPlayer + 4, 60);
    deriveRelationshipStatsFromOpinion(landlordState);
}

bool tryEstablishFamilyFriendDpaHeadquarters(
    PlayerOperationsStore& store,
    PlayerWallet& wallet,
    const PlayerProfile& profile,
    CharacterAgentStore& agentStore,
    uint64_t tickCount) {
    if (hasPlayerHeadquarters(store)) {
        return false;
    }
    if (!hasPersonalLodgingOption(profile.draft)) {
        return false;
    }
    int32_t kinSlotIndex = FRIEND_AGENT_SLOT_INDEX;
    if (profile.draft.hasFamilyInCountry && agentStore.states[FAMILY_AGENT_SLOT_INDEX].isActive) {
        kinSlotIndex = FAMILY_AGENT_SLOT_INDEX;
    } else if (!agentStore.states[FRIEND_AGENT_SLOT_INDEX].isActive) {
        return false;
    }
    int32_t catalogIndex = -1;
    const int32_t catalogCount = getOperationCatalogCount();
    for (int32_t index = 0; index < catalogCount; ++index) {
        const OperationDefinition* operation = getOperationDefinition(index);
        if (operation != nullptr && operation->headquartersKind == HeadquartersKind::FamilyFriendDpa) {
            catalogIndex = index;
            break;
        }
    }
    if (catalogIndex < 0 || !tryEstablishOperation(store, wallet, profile, catalogIndex, tickCount)) {
        return false;
    }
    activateKinLandlordFromDpa(profile, agentStore);
    return true;
}

} // namespace Core
