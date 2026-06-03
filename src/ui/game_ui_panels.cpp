#include "ui/game_ui_panels.h"
#include "game/operation_types.h"
#include "game/player_wallet.h"
#include "sim/sim_event_queue.h"
#include "world/business_node_table.h"
#include "imgui.h"
#include <cstdio>

namespace Core {

namespace {

const char* operationLockReasonToString(OperationLockReason reason) {
    switch (reason) {
    case OperationLockReason::None:
        return "Available";
    case OperationLockReason::NeedsHeadquarters:
        return "Requires headquarters first";
    case OperationLockReason::InsufficientWealth:
        return "Insufficient cash";
    case OperationLockReason::InsufficientNetwork:
        return "Insufficient network access";
    case OperationLockReason::InsufficientReputation:
        return "Insufficient reputation";
    case OperationLockReason::MissingFamilyInCountry:
        return "Requires family in-country";
    case OperationLockReason::AlreadyEstablished:
        return "Already established";
    case OperationLockReason::HeadquartersAlreadySet:
        return "Headquarters already set";
    default:
        return "Locked";
    }
}

} // namespace

void renderOperationsPanel(
    PlayerOperationsStore& playerOperationsStore,
    PlayerWallet& playerWallet,
    SimEventQueue& simEventQueue,
    const PlayerProfile& playerProfile,
    GamePanelVisibility& panelVisibility,
    ContextHelpState& contextHelpState) {
    if (!panelVisibility.showOperations) {
        return;
    }
    ImGui::SetNextWindowSizeConstraints(ImVec2(300.0f, 260.0f), ImVec2(FLT_MAX, FLT_MAX));
    bool isOpen = true;
    if (!ImGui::Begin("Operations", &isOpen)) {
        ImGui::End();
        panelVisibility.showOperations = isOpen;
        return;
    }
    panelVisibility.showOperations = isOpen;
    contextHelpPanelTag("Operations Panel", "Establish headquarters, rackets, and fronts.", "operations_panel", contextHelpState);
    if (!hasPlayerHeadquarters(playerOperationsStore)) {
        ImGui::TextWrapped("Your first operation must be a headquarters: rented room, apartment, or family/friend DPA.");
        ImGui::Separator();
    }
    const int32_t catalogCount = getOperationCatalogCount();
    for (int32_t catalogIndex = 0; catalogIndex < catalogCount; ++catalogIndex) {
        const OperationDefinition* operation = getOperationDefinition(catalogIndex);
        if (operation == nullptr) {
            continue;
        }
        if (hasPlayerHeadquarters(playerOperationsStore) && operation->category == OperationCategory::Headquarters) {
            continue;
        }
        if (!hasPlayerHeadquarters(playerOperationsStore) && operation->category != OperationCategory::Headquarters) {
            continue;
        }
        const OperationLockReason lockReason = evaluateOperationLock(playerOperationsStore, playerProfile, playerWallet, catalogIndex, *operation);
        char costBuffer[32];
        formatCashCents(costBuffer, sizeof(costBuffer), operation->costCents);
        ImGui::PushID(catalogIndex);
        if (lockReason != OperationLockReason::None) {
            ImGui::BeginDisabled();
        }
        if (ImGui::Button(operation->displayName)) {
            pushSimEventWithCatalog(simEventQueue, SimEventType::EstablishOperation, catalogIndex);
        }
        if (lockReason != OperationLockReason::None) {
            ImGui::EndDisabled();
        }
        ImGui::SameLine();
        ImGui::Text("%s | %s", costBuffer, operationLockReasonToString(lockReason));
        ImGui::PopID();
    }
    ImGui::End();
}

void renderBusinessPanel(
    const WorldConfig& worldConfig,
    PlayerOperationsStore& playerOperationsStore,
    PlayerWallet& playerWallet,
    SimEventQueue& simEventQueue,
    const PlayerProfile& playerProfile,
    const ViewportPickState& viewportPickState,
    GamePanelVisibility& panelVisibility,
    ContextHelpState& contextHelpState) {
    if (!panelVisibility.showBusiness) {
        return;
    }
    ImGui::SetNextWindowSizeConstraints(ImVec2(300.0f, 220.0f), ImVec2(FLT_MAX, FLT_MAX));
    bool isOpen = true;
    if (!ImGui::Begin("Business", &isOpen)) {
        ImGui::End();
        panelVisibility.showBusiness = isOpen;
        return;
    }
    panelVisibility.showBusiness = isOpen;
    contextHelpPanelTag("Business Panel", "Blue map nodes: jobs and borough trade.", "business_panel", contextHelpState);
    if (!viewportPickState.hasBusinessSelection || viewportPickState.selectedBusinessIndex < 0) {
        ImGui::TextDisabled("Click a blue business node on the map.");
    } else {
        const BusinessNodeDefinition* business = getBusinessNodeDefinition(viewportPickState.selectedBusinessIndex);
        if (business == nullptr) {
            ImGui::TextDisabled("Invalid business selection.");
        } else {
            ImGui::Text("%s", business->fullName);
            ImGui::Text("Tile: (%d, %d)", business->tileX, business->tileY);
            char wageBuffer[32];
            formatCashCents(wageBuffer, sizeof(wageBuffer), business->jobWageCents);
            ImGui::Text("Hiring bonus: %s", wageBuffer);
            if (getNetworkAccessScore(playerProfile) < business->minNetworkAccess) {
                ImGui::BeginDisabled();
            }
            if (ImGui::Button("Apply for job")) {
                pushSimEventWithJob(simEventQueue, viewportPickState.selectedBusinessIndex);
            }
            if (getNetworkAccessScore(playerProfile) < business->minNetworkAccess) {
                ImGui::EndDisabled();
                ImGui::TextDisabled("Requires higher network access.");
            }
            if (playerOperationsStore.employedBusinessIndex == viewportPickState.selectedBusinessIndex) {
                ImGui::Text("Status: Employed here");
            }
        }
    }
    ImGui::End();
}

void renderContactsPanel(
    const CharacterAgentStore& characterAgentStore,
    GamePanelVisibility& panelVisibility,
    ContextHelpState& contextHelpState) {
    if (!panelVisibility.showContacts) {
        return;
    }
    ImGui::SetNextWindowSizeConstraints(ImVec2(280.0f, 240.0f), ImVec2(FLT_MAX, FLT_MAX));
    bool isOpen = true;
    if (!ImGui::Begin("Contacts", &isOpen)) {
        ImGui::End();
        panelVisibility.showContacts = isOpen;
        return;
    }
    panelVisibility.showContacts = isOpen;
    contextHelpPanelTag("Contacts Panel", "AI characters and their view of you.", "contacts_panel", contextHelpState);
    const int32_t agentCount = getCharacterAgentDefinitionCount();
    for (int32_t agentIndex = 0; agentIndex < agentCount; ++agentIndex) {
        const AgentDefinition* definition = getCharacterAgentDefinition(agentIndex);
        const CharacterAgentState* state = getCharacterAgentState(characterAgentStore, agentIndex);
        if (definition == nullptr || state == nullptr) {
            continue;
        }
        ImGui::Separator();
        ImGui::Text("%s (%s)", definition->displayName, definition->roleLabel);
        ImGui::Text("Opinion: %d | Trust: %d | Respect: %d", state->opinionOfPlayer, state->trust, state->respect);
    }
    ImGui::End();
}

} // namespace Core
