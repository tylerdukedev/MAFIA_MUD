#include "ui/game_ui_panels.h"
#include "game/housing_living_costs.h"
#include "game/player_employment.h"
#include "ui/game_modal_ui.h"
#include "game/kin_housing.h"
#include "world/region_table.h"
#include "sim/world_event_store.h"
#include "game/operation_types.h"
#include "game/player_wallet.h"
#include "game/street_crime.h"
#include "game/player_criminal_justice.h"
#include "game/player_organization.h"
#include "game/player_organization_ui.h"
#include "game/economy_constants.h"
#include "ui/game_modal_ui.h"
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

void renderStreetCrimeTierGroup(
    StreetCrimeTier tier,
    PlayerOperationsStore& playerOperationsStore,
    const PlayerOrganizationStore& playerOrganizationStore,
    PlayerStreetCrimeStore& playerStreetCrimeStore,
    const PlayerLawEnforcementStore& playerLawEnforcementStore,
    const PlayerCriminalJusticeStore& playerCriminalJusticeStore,
    const PlayerProfile& playerProfile,
    const CharacterAgentStore& characterAgentStore,
    SimEventQueue& simEventQueue,
    uint64_t tickCount) {
    ImGui::TextDisabled("%s crimes", streetCrimeTierToString(tier));
    const int32_t crimeCount = getStreetCrimeCount();
    for (int32_t crimeIndex = 0; crimeIndex < crimeCount; ++crimeIndex) {
        const StreetCrimeDefinition* crime = getStreetCrimeDefinition(crimeIndex);
        if (crime == nullptr || crime->tier != tier) {
            continue;
        }
        const StreetCrimeLockReason lockReason = evaluateStreetCrimeLock(
            playerOperationsStore,
            playerStreetCrimeStore,
            playerLawEnforcementStore,
            playerCriminalJusticeStore,
            playerOrganizationStore,
            playerProfile,
            characterAgentStore,
            crimeIndex,
            *crime,
            tickCount);
        ImGui::PushID(crimeIndex + 2000);
        if (lockReason != StreetCrimeLockReason::None) {
            ImGui::BeginDisabled();
        }
        if (ImGui::Button(crime->displayName)) {
            pushSimEventWithCatalog(simEventQueue, SimEventType::CommitStreetCrime, crimeIndex);
        }
        if (lockReason != StreetCrimeLockReason::None) {
            ImGui::EndDisabled();
        }
        ImGui::SameLine();
        ImGui::Text("%s", streetCrimeLockReasonToString(lockReason));
        ImGui::TextWrapped("%s", crime->description);
        ImGui::PopID();
    }
}

} // namespace

void renderOperationsPanel(
    PlayerOperationsStore& playerOperationsStore,
    PlayerOrganizationStore& playerOrganizationStore,
    PlayerStreetCrimeStore& playerStreetCrimeStore,
    PlayerLawEnforcementStore& playerLawEnforcementStore,
    PlayerCriminalJusticeStore& playerCriminalJusticeStore,
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
        ImGui::TextWrapped("Your first operation must be a headquarters: rented room, apartment, or family/friend stay (DPA).");
        ImGui::TextDisabled("Move-in deposit is paid once; rent, taxes, and utilities bill monthly.");
        ImGui::Separator();
    } else {
        MonthlyHousingLedger ledger{};
        buildMonthlyHousingLedger(playerOperationsStore, playerOperationsStore.employedBusinessIndex, ledger);
        if (playerOperationsStore.headquartersKind == HeadquartersKind::FamilyFriendDpa) {
            ImGui::TextWrapped("Family/friend stay (DPA): no cash rent, but relationships erode slowly each month unless you pitch in.");
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
            } else if (operation->headquartersKind == HeadquartersKind::FamilyFriendDpa) {
                tryEstablishFamilyFriendDpaHeadquarters(
                    playerOperationsStore, playerWallet, playerProfile, characterAgentStore, tickCount);
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
    ImGui::Separator();
    contextHelpSectionHeader(
        "Power tier",
        "Solo → Crew (street gang) → Organization (official enterprise).",
        "power_tier",
        contextHelpState);
    ImGui::Text("Tier: %s", playerPowerTierToString(playerOrganizationStore.powerTier));
    if (playerOrganizationStore.powerTier == PlayerPowerTier::Solo) {
        ImGui::Text("Recruits: %d / %d", playerOrganizationStore.crewMemberCount, MAX_CREW_MEMBER_COUNT);
        if (playerOrganizationStore.crewMemberCount >= MIN_CREW_MEMBERS_TO_FORM) {
            if (ImGui::Button("Formalize crew")) {
                beginCrewFormalizeModal(gameModalState, simClock);
            }
        }
    } else if (playerOrganizationStore.powerTier == PlayerPowerTier::Crew) {
        ImGui::Text("Crew: %s", playerOrganizationStore.crewName);
        const OrganizationFormLockReason orgLock = evaluateOrganizationFormLock(
            playerOrganizationStore, playerLawEnforcementStore, playerCriminalJusticeStore, playerProfile, playerWallet);
        ImGui::TextDisabled("%s", organizationFormLockReasonToString(orgLock));
        if (ImGui::Button("Incorporate organization")) {
            beginOrganizationCreationModal(gameModalState, simClock);
        }
    } else {
        ImGui::Text("Org: %s", playerOrganizationStore.organizationName);
        ImGui::TextDisabled("Front: %s", playerOrganizationStore.organizationFront);
    }
    ImGui::Separator();
    contextHelpSectionHeader(
        "Street crime",
        "Solo cash when broke; crew and organization jobs need trusted criminals and network.",
        "street_crime_panel",
        contextHelpState);
    if (getPlayerCustodyPhase(playerCriminalJusticeStore) != CustodyPhase::Free) {
        ImGui::TextColored(ImVec4(0.95f, 0.45f, 0.35f, 1.0f), "Custody: %s", custodyPhaseToString(getPlayerCustodyPhase(playerCriminalJusticeStore)));
        if (playerCriminalJusticeStore.lastCustodyLabel[0] != '\0') {
            ImGui::TextDisabled("%s", playerCriminalJusticeStore.lastCustodyLabel);
        }
    }
    ImGui::Text(
        "Heat: %d | %s | Evidence: %d | Warrants: %d",
        playerLawEnforcementStore.personalHeat,
        getPoliceInvestigationLabel(playerLawEnforcementStore.investigationTier),
        playerLawEnforcementStore.evidenceScore,
        playerLawEnforcementStore.activeWarrantCount);
    if (playerLawEnforcementStore.witnessCount > 0) {
        ImGui::TextDisabled("Witnesses on record: %d (%s)", playerLawEnforcementStore.witnessCount, playerLawEnforcementStore.lastWitnessLabel);
    }
    ImGui::Text("Criminal trust (best): %d", computeBestCriminalContactTrust(characterAgentStore));
    if (playerWallet.cashCents <= STREET_CRIME_BROKE_CASH_THRESHOLD_CENTS) {
        ImGui::TextColored(ImVec4(0.95f, 0.75f, 0.35f, 1.0f), "Low cash — solo crimes are your fastest option.");
    }
    if (!hasPlayerHeadquarters(playerOperationsStore)) {
        ImGui::TextDisabled("Establish a headquarters before street work.");
    } else {
        renderStreetCrimeTierGroup(
            StreetCrimeTier::Solo,
            playerOperationsStore,
            playerOrganizationStore,
            playerStreetCrimeStore,
            playerLawEnforcementStore,
            playerCriminalJusticeStore,
            playerProfile,
            characterAgentStore,
            simEventQueue,
            tickCount);
        renderStreetCrimeTierGroup(
            StreetCrimeTier::Crew,
            playerOperationsStore,
            playerOrganizationStore,
            playerStreetCrimeStore,
            playerLawEnforcementStore,
            playerCriminalJusticeStore,
            playerProfile,
            characterAgentStore,
            simEventQueue,
            tickCount);
        renderStreetCrimeTierGroup(
            StreetCrimeTier::Organization,
            playerOperationsStore,
            playerOrganizationStore,
            playerStreetCrimeStore,
            playerLawEnforcementStore,
            playerCriminalJusticeStore,
            playerProfile,
            characterAgentStore,
            simEventQueue,
            tickCount);
        ImGui::TextDisabled("Bigger scores will tie into racket upkeep and crew payroll in later builds.");
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
    const PlayerOrganizationStore& playerOrganizationStore,
    GameModalState& gameModalState,
    SimClock& simClock,
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
        ImGui::Text("Loyalty score: %d", computeAgentLoyaltyScore(*state));
        if (playerOrganizationStore.powerTier != PlayerPowerTier::Organization) {
            const CrewRecruitLockReason recruitLock = evaluateCrewRecruitLock(playerOrganizationStore, characterAgentStore, agentIndex);
            ImGui::PushID(agentIndex + 4000);
            if (recruitLock != CrewRecruitLockReason::None) {
                ImGui::BeginDisabled();
            }
            if (ImGui::Button("Talk: recruit to crew")) {
                beginCrewRecruitmentModal(gameModalState, agentIndex, simClock);
            }
            if (recruitLock != CrewRecruitLockReason::None) {
                ImGui::EndDisabled();
            }
            ImGui::SameLine();
            ImGui::TextDisabled("%s", crewRecruitLockReasonToString(recruitLock));
            ImGui::PopID();
        }
    }
    ImGui::End();
}

} // namespace Core
