#include "ui/game_ui_panels.h"
#include "game/housing_living_costs.h"
#include "game/player_employment.h"
#include "ui/game_modal_ui.h"
#include "world/region_table.h"
#include "sim/world_event_store.h"
#include "game/operation_types.h"
#include "game/player_wallet.h"
#include "sim/character_agent.h"
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
        return "Requires family or friend in-country";
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
    CharacterAgentStore& characterAgentStore,
    const WorldEventStore& worldEventStore,
    SimEventQueue& simEventQueue,
    const PlayerProfile& playerProfile,
    PlayerWorldState& playerWorldState,
    GameModalState& gameModalState,
    SimClock& simClock,
    uint64_t tickCount,
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
        ImGui::TextDisabled("Move-in deposit is paid once; rent, taxes, and utilities bill monthly.");
        ImGui::Separator();
    } else {
        MonthlyHousingLedger ledger{};
        buildMonthlyHousingLedger(playerOperationsStore, playerOperationsStore.employedBusinessIndex, ledger);
        if (playerOperationsStore.headquartersKind == HeadquartersKind::FamilyFriendDpa) {
            ImGui::TextWrapped("Family/friend DPA: no cash rent, but relationships erode slowly each month unless you pitch in.");
        } else {
            char expenseBuffer[32];
            formatCashCents(expenseBuffer, sizeof(expenseBuffer), ledger.totalExpenseCents);
            ImGui::Text("Monthly housing due: %s", expenseBuffer);
            const int32_t effectiveRentBps = computeEffectiveRentMultiplierBps(playerOperationsStore);
            ImGui::TextDisabled("Rent index: %.0f%% (borough economy + your influence)", static_cast<double>(effectiveRentBps) / 100.0);
            if (playerOperationsStore.consecutiveUnpaidRentMonths > 0) {
                ImGui::TextColored(ImVec4(0.95f, 0.45f, 0.35f, 1.0f), "Missed rent months: %d", static_cast<int>(playerOperationsStore.consecutiveUnpaidRentMonths));
            }
            if (ledger.rentCents > 0) {
                char rentBuffer[32];
                formatCashCents(rentBuffer, sizeof(rentBuffer), ledger.rentCents);
                ImGui::BulletText("Rent: %s", rentBuffer);
            }
            if (ledger.utilitiesCents > 0) {
                char utilBuffer[32];
                formatCashCents(utilBuffer, sizeof(utilBuffer), ledger.utilitiesCents);
                ImGui::BulletText("Gas, water, sewer, electric: %s", utilBuffer);
            }
            if (ledger.taxesAndFeesCents > 0) {
                char taxBuffer[32];
                formatCashCents(taxBuffer, sizeof(taxBuffer), ledger.taxesAndFeesCents);
                ImGui::BulletText("City taxes & fees: %s", taxBuffer);
            }
        }
        if (ledger.jobIncomeCents > 0) {
            char wageBuffer[32];
            formatCashCents(wageBuffer, sizeof(wageBuffer), ledger.jobIncomeCents);
            ImGui::BulletText("Job wages (monthly): %s", wageBuffer);
        }
        ImGui::Separator();
        if (playerOperationsStore.headquartersKind == HeadquartersKind::FamilyFriendDpa) {
            ImGui::Text("Household upkeep (short cooldown):");
            const int32_t upkeepCount = getFamilyUpkeepActionCount();
            for (int32_t actionIndex = 0; actionIndex < upkeepCount; ++actionIndex) {
                const FamilyUpkeepActionDefinition* action = getFamilyUpkeepActionDefinition(actionIndex);
                if (action == nullptr) {
                    continue;
                }
                if (!characterAgentStore.states[action->targetAgentIndex].isActive) {
                    continue;
                }
                char costBuffer[32];
                formatCashCents(costBuffer, sizeof(costBuffer), action->costCents);
                ImGui::PushID(actionIndex + 1000);
                if (!canApplyFamilyUpkeep(playerOperationsStore, tickCount)) {
                    ImGui::BeginDisabled();
                }
                if (ImGui::Button(action->displayName)) {
                    tryApplyFamilyUpkeep(playerOperationsStore, playerWallet, characterAgentStore, actionIndex, tickCount);
                }
                if (!canApplyFamilyUpkeep(playerOperationsStore, tickCount)) {
                    ImGui::EndDisabled();
                }
                ImGui::SameLine();
                ImGui::Text("%s | +%d opinion", costBuffer, action->opinionBonus);
                ImGui::PopID();
            }
            ImGui::Separator();
        }
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
            if (operation->headquartersKind == HeadquartersKind::Apartment) {
                beginApartmentApplicationModal(gameModalState, catalogIndex, simClock);
            } else {
                pushSimEventWithCatalog(simEventQueue, SimEventType::EstablishOperation, catalogIndex);
            }
        }
        if (lockReason != OperationLockReason::None) {
            ImGui::EndDisabled();
        }
        ImGui::SameLine();
        if (operation->category == OperationCategory::Headquarters && operation->headquartersKind != HeadquartersKind::FamilyFriendDpa) {
            ImGui::Text("%s deposit | %s", costBuffer, operationLockReasonToString(lockReason));
        } else if (operation->headquartersKind == HeadquartersKind::FamilyFriendDpa) {
            ImGui::Text("no deposit | %s", operationLockReasonToString(lockReason));
        } else {
            ImGui::Text("%s | %s", costBuffer, operationLockReasonToString(lockReason));
        }
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
    const PlayerWorldState& playerWorldState,
    GameModalState& gameModalState,
    SimClock& simClock,
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
            ImGui::Text("Monthly wage (est.): %s", wageBuffer);
            const RegionId businessRegion = getBusinessNodeRegionId(viewportPickState.selectedBusinessIndex);
            const bool isInRegion = canPlayerOperateInRegion(playerWorldState, businessRegion);
            if (!isInRegion) {
                ImGui::TextColored(ImVec4(0.95f, 0.55f, 0.35f, 1.0f), "You must be in %s to apply.", RegionTable::getRegionShortName(businessRegion).data());
            }
            if (getNetworkAccessScore(playerProfile) < business->minNetworkAccess) {
                ImGui::BeginDisabled();
            }
            if (!isInRegion || isPlayerEmployed(playerOperationsStore)) {
                ImGui::BeginDisabled();
            }
            if (ImGui::Button("Apply for job")) {
                beginJobInterviewModal(gameModalState, viewportPickState.selectedBusinessIndex, simClock);
            }
            if (!isInRegion || isPlayerEmployed(playerOperationsStore)) {
                ImGui::EndDisabled();
            }
            if (getNetworkAccessScore(playerProfile) < business->minNetworkAccess) {
                ImGui::EndDisabled();
                ImGui::TextDisabled("Requires higher network access.");
            }
            if (isPlayerEmployed(playerOperationsStore) && playerOperationsStore.employedBusinessIndex == viewportPickState.selectedBusinessIndex) {
                ImGui::Text("Status: Employed here");
            }
            if (playerWorldState.isAtWork) {
                ImGui::TextDisabled("Currently on shift — limited actions.");
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
    for (int32_t agentIndex = 0; agentIndex < MAX_CHARACTER_AGENT_COUNT; ++agentIndex) {
        const CharacterAgentState* state = getCharacterAgentState(characterAgentStore, agentIndex);
        const char* displayName = nullptr;
        const char* roleLabel = nullptr;
        if (state == nullptr || !tryGetAgentDisplayLabels(characterAgentStore, agentIndex, displayName, roleLabel)) {
            continue;
        }
        ImGui::Separator();
        ImGui::Text("%s (%s)", displayName, roleLabel);
        ImGui::Text("Opinion: %d | Trust: %d | Respect: %d", state->opinionOfPlayer, state->trust, state->respect);
    }
    ImGui::End();
}

} // namespace Core
