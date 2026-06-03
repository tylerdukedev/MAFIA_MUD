#include "sim/operation_system.h"
#include "character/character_social_network.h"
#include "game/operation_types.h"
#include "game/player_operations.h"
#include "sim/character_agent.h"
#include "world/business_node_table.h"

namespace Core {

void OperationSystem::bind(const SimWorldBindings& inputBindings, CharacterAgentStore* inputAgentStore) {
    bindings = inputBindings;
    agentStore = inputAgentStore;
}

const char* OperationSystem::getName() const {
    return "OperationSystem";
}

void OperationSystem::processEstablishOperationEvent(const SimEvent& event, uint64_t tickCount) {
    if (bindings.playerOperationsStore == nullptr || bindings.playerWallet == nullptr || bindings.playerProfile == nullptr) {
        return;
    }
    PlayerOperationsStore& store = *bindings.playerOperationsStore;
    const OperationDefinition* operation = getOperationDefinition(event.catalogIndex);
    if (operation == nullptr) {
        return;
    }
    if (!tryEstablishOperation(store, *bindings.playerWallet, *bindings.playerProfile, event.catalogIndex, tickCount)) {
        return;
    }
    if (operation->headquartersKind != HeadquartersKind::FamilyFriendDpa || agentStore == nullptr) {
        return;
    }
    if (agentStore->states[FAMILY_AGENT_SLOT_INDEX].isActive) {
        adjustAgentOpinion(*agentStore, FAMILY_AGENT_SLOT_INDEX, -store.familyOpinionPenalty);
    }
    if (agentStore->states[FRIEND_AGENT_SLOT_INDEX].isActive) {
        adjustAgentOpinion(*agentStore, FRIEND_AGENT_SLOT_INDEX, -(store.familyOpinionPenalty / 2));
    }
}

void OperationSystem::processApplyForJobEvent(const SimEvent& event) {
    if (bindings.playerOperationsStore == nullptr || bindings.playerWallet == nullptr || bindings.playerProfile == nullptr) {
        return;
    }
    const BusinessNodeDefinition* business = getBusinessNodeDefinition(event.businessNodeIndex);
    if (business == nullptr) {
        return;
    }
    if (getNetworkAccessScore(*bindings.playerProfile) < business->minNetworkAccess) {
        return;
    }
    PlayerOperationsStore& store = *bindings.playerOperationsStore;
    store.employedBusinessIndex = event.businessNodeIndex;
    creditLegitCash(*bindings.playerWallet, business->jobWageCents);
}

void OperationSystem::onTick(uint64_t tickCount) {
    lastTickCount = tickCount;
    if (bindings.eventQueue == nullptr) {
        return;
    }
    SimEvent event{};
    while (popSimEvent(*bindings.eventQueue, event)) {
        if (event.type == SimEventType::EstablishOperation) {
            processEstablishOperationEvent(event, tickCount);
        }
        if (event.type == SimEventType::ApplyForJob) {
            processApplyForJobEvent(event);
        }
    }
}

} // namespace Core
