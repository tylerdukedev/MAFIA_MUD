#include "ui/game_ui_panels.h"
#include "ui/game_dock_panels.h"
#include "game/housing_living_costs.h"
#include "game/player_employment.h"
#include "ui/game_modal_ui.h"
#include "game/kin_housing.h"
#include "game/landlord_contact.h"
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
#include "game/npc_population.h"
#include "sim/sim_event_queue.h"
#include "world/business_node_table.h"
#include "game/job_catalog.h"
#include "game/legal_counsel.h"
#include "game/player_law_intel.h"
#include "game/player_information_feed.h"
#include "game/agent_relation_events.h"
#include "game/social_action_catalog.h"
#include "ui/real_estate_ui.h"
#include "game/travel_modes.h"
#include "game/game_calendar.h"
#include "game/player_work_schedule.h"
#include "game/player_employment.h"
#include "ui/dock_panel_helpers.h"
#include "imgui.h"
#include <cstdio>

namespace Core {

namespace {

constexpr int32_t OPERATIONS_SECTION_HOUSING = 0;
constexpr int32_t OPERATIONS_SECTION_HQ = 1;
constexpr int32_t OPERATIONS_SECTION_POWER = 2;
constexpr int32_t OPERATIONS_SECTION_STREET_CRIME = 3;
constexpr int32_t OPERATIONS_SECTION_INFORMATION = 4;
constexpr int32_t OPERATIONS_SECTION_NARRATIVE = 5;
constexpr int32_t OPERATIONS_SECTION_LEGAL = 6;
constexpr int32_t OPERATIONS_SECTION_TRAVEL = 7;

constexpr int32_t BUSINESS_SECTION_CALENDAR = 0;
constexpr int32_t BUSINESS_SECTION_EMPLOYMENT = 1;
constexpr int32_t BUSINESS_SECTION_SELECTED = 2;
constexpr int32_t BUSINESS_SECTION_APPLY = 3;
constexpr int32_t BUSINESS_SECTION_STATUS = 4;

constexpr int32_t CONTACTS_SECTION_HEADER = 0;
constexpr int32_t CONTACTS_SECTION_ACTIONS = 1;
constexpr int32_t CONTACTS_SECTION_PRESSURE = 2;
constexpr int32_t CONTACTS_SECTION_CREW = 3;
constexpr int32_t CONTACTS_SECTION_INLINE = 4;
constexpr int32_t CONTACTS_SECTION_TAIL = 5;

void swapInt32(int32_t& a, int32_t& b) {
    const int32_t temp = a;
    a = b;
    b = temp;
}

void moveOrderEarlier(int32_t* order, int32_t count, int32_t sectionIndex) {
    if (sectionIndex <= 0 || sectionIndex >= count) {
        return;
    }
    swapInt32(order[sectionIndex], order[sectionIndex - 1]);
}

void moveOrderLater(int32_t* order, int32_t count, int32_t sectionIndex) {
    if (sectionIndex < 0 || sectionIndex + 1 >= count) {
        return;
    }
    swapInt32(order[sectionIndex], order[sectionIndex + 1]);
}

void renderReorderControls(int32_t* order, int32_t count, int32_t sectionIndex, const char* label) {
    ImGui::PushID(label);
    ImGui::SameLine();
    if (ImGui::SmallButton("↑")) {
        moveOrderEarlier(order, count, sectionIndex);
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("↓")) {
        moveOrderLater(order, count, sectionIndex);
    }
    ImGui::PopID();
}

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

void renderSurvivalStreetCrimes(
    PlayerOperationsStore& playerOperationsStore,
    const PlayerOrganizationStore& playerOrganizationStore,
    PlayerStreetCrimeStore& playerStreetCrimeStore,
    const PlayerLawEnforcementStore& playerLawEnforcementStore,
    const PlayerCriminalJusticeStore& playerCriminalJusticeStore,
    const PlayerProfile& playerProfile,
    const CharacterAgentStore& characterAgentStore,
    SimEventQueue& simEventQueue,
    uint64_t tickCount) {
    ImGui::TextDisabled("Survival street work (no HQ required)");
    const int32_t crimeCount = getStreetCrimeCount();
    for (int32_t crimeIndex = 0; crimeIndex < crimeCount; ++crimeIndex) {
        const StreetCrimeDefinition* crime = getStreetCrimeDefinition(crimeIndex);
        if (crime == nullptr || !crime->allowsWithoutHeadquarters) {
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
        ImGui::PushID(crimeIndex + 3000);
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
        ImGui::PopID();
    }
}

int32_t findOperationsSectionIndex(const GamePanelVisibility& panelVisibility, int32_t sectionId) {
    for (int32_t index = 0; index < OPERATIONS_PANEL_SECTION_COUNT; ++index) {
        if (panelVisibility.operationsSectionOrder[index] == sectionId) {
            return index;
        }
    }
    return -1;
}

void renderOperationsSectionHeader(GamePanelVisibility& panelVisibility, int32_t sectionId, const char* label) {
    const int32_t displayIndex = findOperationsSectionIndex(panelVisibility, sectionId);
    ImGui::Separator();
    ImGui::TextUnformatted(label);
    if (displayIndex >= 0) {
        ImGui::SameLine();
        ImGui::PushID(label);
        if (ImGui::SmallButton("↑") && displayIndex > 0) {
            swapInt32(panelVisibility.operationsSectionOrder[displayIndex], panelVisibility.operationsSectionOrder[displayIndex - 1]);
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("↓") && displayIndex + 1 < OPERATIONS_PANEL_SECTION_COUNT) {
            swapInt32(panelVisibility.operationsSectionOrder[displayIndex], panelVisibility.operationsSectionOrder[displayIndex + 1]);
        }
        ImGui::PopID();
    }
}

void renderOperationsHousingSection(
    PlayerOperationsStore& playerOperationsStore,
    PlayerWorldState& playerWorldState,
    CharacterAgentStore& characterAgentStore,
    PlayerWallet& playerWallet,
    uint64_t tickCount,
    bool isAtWorkRestricted) {
    if (isAtWorkRestricted) {
        ImGui::TextColored(ImVec4(0.55f, 0.85f, 1.0f, 1.0f), "On shift — limited panel access. Simulation keeps running.");
        ImGui::TextDisabled("Street ops and travel are locked. Contacts and map events can still reach you.");
        return;
    }
    if (!hasPlayerHeadquarters(playerOperationsStore)) {
        ImGui::TextWrapped("Claim a rented room or apartment as your headquarters — your first home is your base of operations.");
        ImGui::TextDisabled("Move-in deposit is paid once; rent, taxes, and utilities bill monthly. Family stay (DPA) is optional if you have kin in-country.");
        return;
    }
    MonthlyHousingLedger ledger{};
    buildMonthlyHousingLedger(playerOperationsStore, playerOperationsStore.employedBusinessIndices[0], ledger);
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
            if (action == nullptr || !characterAgentStore.states[action->targetAgentIndex].isActive) {
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

void renderOperationsPowerSection(
    const PlayerOrganizationStore& playerOrganizationStore,
    PlayerCriminalJusticeStore& playerCriminalJusticeStore,
    PlayerLawEnforcementStore& playerLawEnforcementStore,
    const PlayerProfile& playerProfile,
    PlayerWallet& playerWallet,
    GameModalState& gameModalState,
    SimClock& simClock,
    ContextHelpState& contextHelpState) {
    contextHelpSectionHeader(
        "Power tier",
        "Solo → Crew (street gang) → Organization (official enterprise).",
        "power_tier",
        contextHelpState);
    ImGui::Text("Tier: %s", playerPowerTierToString(playerOrganizationStore.powerTier));
    if (playerOrganizationStore.powerTier == PlayerPowerTier::Solo) {
        ImGui::Text("Recruits: %d / %d", playerOrganizationStore.crewMemberCount, MAX_CREW_MEMBER_COUNT);
        if (playerOrganizationStore.crewMemberCount >= MIN_CREW_MEMBERS_TO_FORM && ImGui::Button("Formalize crew")) {
            beginCrewFormalizeModal(gameModalState, simClock);
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
}

void renderOperationsStreetCrimeSection(
    PlayerOperationsStore& playerOperationsStore,
    const PlayerOrganizationStore& playerOrganizationStore,
    PlayerStreetCrimeStore& playerStreetCrimeStore,
    PlayerLawEnforcementStore& playerLawEnforcementStore,
    PlayerCriminalJusticeStore& playerCriminalJusticeStore,
    const PlayerProfile& playerProfile,
    CharacterAgentStore& characterAgentStore,
    PlayerWallet& playerWallet,
    SimEventQueue& simEventQueue,
    uint64_t tickCount,
    bool isAtWorkRestricted,
    ContextHelpState& contextHelpState) {
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
        "Heat: %d | %s",
        playerLawEnforcementStore.personalHeat,
        getPoliceInvestigationLabel(playerLawEnforcementStore.investigationTier));
    if (playerWallet.cashCents <= STREET_CRIME_BROKE_CASH_THRESHOLD_CENTS) {
        ImGui::TextColored(ImVec4(0.95f, 0.75f, 0.35f, 1.0f), "Low cash — solo crimes are your fastest option.");
    }
    if (!isAtWorkRestricted && !hasPlayerHeadquarters(playerOperationsStore)) {
        renderSurvivalStreetCrimes(
            playerOperationsStore,
            playerOrganizationStore,
            playerStreetCrimeStore,
            playerLawEnforcementStore,
            playerCriminalJusticeStore,
            playerProfile,
            characterAgentStore,
            simEventQueue,
            tickCount);
    }
}

void renderOperationsInfoSection(const PlayerInformationFeedStore& informationFeedStore, ContextHelpState& contextHelpState) {
    contextHelpSectionHeader("Information", "Newspapers, rumors, and network intel.", "law_intel", contextHelpState);
    ImGui::Text("Unread items: %d", getUnreadInformationFeedCount(informationFeedStore));
    ImGui::TextDisabled("Map alerts appear bottom-right; newspapers pause sim when opened.");
}

void renderOperationsNarrativeSection(const PlayerNarrativeArchiveStore& narrativeArchiveStore, ContextHelpState& contextHelpState) {
    contextHelpSectionHeader("Narrative archive", "Logged motives for headlines and former-life collectibles.", "law_intel", contextHelpState);
    ImGui::Text("Story beats recorded: %d", getNarrativeBeatCount(narrativeArchiveStore));
    const int32_t beatCount = getNarrativeBeatCount(narrativeArchiveStore);
    if (beatCount > 0) {
        ImGui::TextDisabled("Latest: %s", narrativeArchiveStore.beats[beatCount - 1].headline);
    }
}

void renderOperationsLegalSection(
    PlayerLegalCounselStore& legalCounselStore,
    PlayerWallet& playerWallet,
    ContextHelpState& contextHelpState) {
    contextHelpSectionHeader("Legal counsel", "Hire a lawyer before court for better outcomes.", "legal_counsel", contextHelpState);
    const LawyerTierDefinition* activeLawyer = getLawyerTierDefinition(legalCounselStore.hiredLawyerTier);
    ImGui::Text("Retained: %s", activeLawyer->displayName);
    for (int32_t tierIndex = 0; tierIndex < getLawyerTierCount(); ++tierIndex) {
        const LawyerTier tier = static_cast<LawyerTier>(tierIndex);
        const LawyerTierDefinition* tierDef = getLawyerTierDefinition(tier);
        char hireLabel[64];
        char costBuffer[32];
        formatCashCents(costBuffer, sizeof(costBuffer), tierDef->retainerCents);
        std::snprintf(hireLabel, sizeof(hireLabel), "Hire %s (%s)", tierDef->displayName, costBuffer);
        if (ImGui::Button(hireLabel)) {
            tryHireLawyer(legalCounselStore, playerWallet, tier);
        }
    }
}

void renderOperationsTravelSection(
    const WorldConfig& worldConfig,
    const ChunkStore& chunkStore,
    PlayerWallet& playerWallet,
    PlayerWorldState& playerWorldState,
    const ViewportPickState& viewportPickState,
    GamePanelVisibility& panelVisibility,
    uint64_t tickCount,
    ContextHelpState& contextHelpState) {
    contextHelpSectionHeader("Travel", "Move between boroughs with realistic time costs.", "travel_modes", contextHelpState);
    ImGui::Text("Location: tile (%d, %d)", playerWorldState.currentTileX, playerWorldState.currentTileY);
    if (playerWorldState.isTraveling) {
        ImGui::TextDisabled("In transit...");
    }
    if (viewportPickState.hasSelection) {
        panelVisibility.travelTargetX = viewportPickState.selectedCoord.x;
        panelVisibility.travelTargetY = viewportPickState.selectedCoord.y;
    }
    ImGui::InputInt("Target X", &panelVisibility.travelTargetX);
    ImGui::InputInt("Target Y", &panelVisibility.travelTargetY);
    if (viewportPickState.hasSelection) {
        ImGui::TextDisabled(
            "Target from map click: (%d, %d)",
            panelVisibility.travelTargetX,
            panelVisibility.travelTargetY);
    }
    ImGui::Combo("Mode", &panelVisibility.travelModeIndex, "Walk\0Bicycle\0Car\0Train\0");
    if (ImGui::Button("Plan / execute travel")) {
        TravelPlan plan{};
        const TravelMode mode = static_cast<TravelMode>(panelVisibility.travelModeIndex);
        buildTravelPlan(
            plan,
            mode,
            playerWorldState.currentTileX,
            playerWorldState.currentTileY,
            panelVisibility.travelTargetX,
            panelVisibility.travelTargetY,
            chunkStore,
            worldConfig);
        const WorldCoord targetCoord{panelVisibility.travelTargetX, panelVisibility.travelTargetY};
        const RegionId targetRegion = chunkStore.getRegionAt(targetCoord);
        tryExecuteTravelPlan(plan, playerWorldState, targetRegion, tickCount, playerWallet, chunkStore, worldConfig);
    }
}

void renderOperationsSectionById(
    int32_t sectionId,
    const GameUiFrameContext& frame,
    GameModalState& gameModalState,
    bool isAtWorkRestricted,
    ContextHelpState& contextHelpState) {
    const WorldConfig& worldConfig = frame.worldConfig;
    const ChunkStore& chunkStore = frame.chunkStore;
    PlayerOperationsStore& playerOperationsStore = frame.playerOperationsStore;
    PlayerOrganizationStore& playerOrganizationStore = frame.playerOrganizationStore;
    PlayerStreetCrimeStore& playerStreetCrimeStore = frame.playerStreetCrimeStore;
    PlayerLawEnforcementStore& playerLawEnforcementStore = frame.playerLawEnforcementStore;
    PlayerLawIntelStore& playerLawIntelStore = frame.gameplayStores.lawIntelStore;
    const PlayerInformationFeedStore& informationFeedStore = frame.gameplayStores.informationFeedStore;
    PlayerCriminalJusticeStore& playerCriminalJusticeStore = frame.playerCriminalJusticeStore;
    PlayerLegalCounselStore& legalCounselStore = frame.gameplayStores.legalCounselStore;
    const PlayerNarrativeArchiveStore& narrativeArchiveStore = frame.gameplayStores.narrativeArchiveStore;
    PlayerWallet& playerWallet = frame.playerWallet;
    PlayerWorldState& playerWorldState = frame.playerWorldState;
    CharacterAgentStore& characterAgentStore = frame.characterAgentStore;
    SimEventQueue& simEventQueue = frame.simEventQueue;
    const PlayerProfile& playerProfile = frame.playerProfile;
    SimClock& simClock = frame.simClock;
    const uint64_t tickCount = frame.tickCount;
    const ViewportPickState& viewportPickState = frame.viewportPickState;
    GamePanelVisibility& panelVisibility = frame.panelVisibility;
    switch (sectionId) {
    case OPERATIONS_SECTION_HOUSING:
        renderOperationsSectionHeader(panelVisibility, sectionId, "Housing / HQ");
        renderOperationsHousingSection(playerOperationsStore, playerWorldState, characterAgentStore, playerWallet, tickCount, isAtWorkRestricted);
        break;
    case OPERATIONS_SECTION_POWER:
        renderOperationsSectionHeader(panelVisibility, sectionId, "Power tier");
        renderOperationsPowerSection(playerOrganizationStore, playerCriminalJusticeStore, playerLawEnforcementStore, playerProfile, playerWallet, gameModalState, simClock, contextHelpState);
        break;
    case OPERATIONS_SECTION_STREET_CRIME:
        renderOperationsSectionHeader(panelVisibility, sectionId, "Street crime");
        renderOperationsStreetCrimeSection(playerOperationsStore, playerOrganizationStore, playerStreetCrimeStore, playerLawEnforcementStore, playerCriminalJusticeStore, playerProfile, characterAgentStore, playerWallet, simEventQueue, tickCount, isAtWorkRestricted, contextHelpState);
        break;
    case OPERATIONS_SECTION_INFORMATION:
        renderOperationsSectionHeader(panelVisibility, sectionId, "Information");
        renderOperationsInfoSection(informationFeedStore, contextHelpState);
        break;
    case OPERATIONS_SECTION_NARRATIVE:
        renderOperationsSectionHeader(panelVisibility, sectionId, "Narrative archive");
        renderOperationsNarrativeSection(narrativeArchiveStore, contextHelpState);
        break;
    case OPERATIONS_SECTION_LEGAL:
        renderOperationsSectionHeader(panelVisibility, sectionId, "Legal counsel");
        renderOperationsLegalSection(legalCounselStore, playerWallet, contextHelpState);
        break;
    case OPERATIONS_SECTION_TRAVEL:
        if (!isAtWorkRestricted) {
            renderOperationsSectionHeader(panelVisibility, sectionId, "Travel");
            renderOperationsTravelSection(worldConfig, chunkStore, playerWallet, playerWorldState, viewportPickState, panelVisibility, tickCount, contextHelpState);
        }
        break;
    default:
        break;
    }
}

void renderOperationsPanelBody(
    const GameUiFrameContext& frame,
    GameModalState& gameModalState,
    bool isAtWorkRestricted) {
    const GamePanelVisibility& panelVisibility = frame.panelVisibility;
    for (int32_t orderIndex = 0; orderIndex < OPERATIONS_PANEL_SECTION_COUNT; ++orderIndex) {
        const int32_t sectionId = panelVisibility.operationsSectionOrder[orderIndex];
        if (sectionId == OPERATIONS_SECTION_HOUSING) {
            renderOperationsSectionById(sectionId, frame, gameModalState, isAtWorkRestricted, frame.contextHelpState);
            renderPropertyBrowserSection(
                frame.propertyListingStore,
                frame.propertyStore,
                frame.bankLoanStore,
                frame.playerOperationsStore,
                frame.playerWallet,
                frame.playerProfile,
                frame.gameplayStores.calendarStore,
                frame.boroughVitalityStore,
                frame.tickCount);
        } else {
            renderOperationsSectionById(sectionId, frame, gameModalState, isAtWorkRestricted, frame.contextHelpState);
        }
    }
}

int32_t findBusinessSectionIndex(const GamePanelVisibility& panelVisibility, int32_t sectionId) {
    for (int32_t index = 0; index < BUSINESS_PANEL_SECTION_COUNT; ++index) {
        if (panelVisibility.businessSectionOrder[index] == sectionId) {
            return index;
        }
    }
    return -1;
}

void renderBusinessSectionHeader(GamePanelVisibility& panelVisibility, int32_t sectionId, const char* label) {
    const int32_t displayIndex = findBusinessSectionIndex(panelVisibility, sectionId);
    ImGui::Separator();
    ImGui::TextUnformatted(label);
    if (displayIndex >= 0) {
        ImGui::SameLine();
        ImGui::PushID(label);
        if (ImGui::SmallButton("↑") && displayIndex > 0) {
            swapInt32(panelVisibility.businessSectionOrder[displayIndex], panelVisibility.businessSectionOrder[displayIndex - 1]);
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("↓") && displayIndex + 1 < BUSINESS_PANEL_SECTION_COUNT) {
            swapInt32(panelVisibility.businessSectionOrder[displayIndex], panelVisibility.businessSectionOrder[displayIndex + 1]);
        }
        ImGui::PopID();
    }
}

void renderBusinessCalendarSection(const GameUiFrameContext& frame) {
    const PlayerOperationsStore& playerOperationsStore = frame.playerOperationsStore;
    const GameCalendarStore& calendarStore = frame.gameplayStores.calendarStore;
    const PlayerWorkScheduleStore& workScheduleStore = frame.gameplayStores.workScheduleStore;
    renderBusinessSectionHeader(frame.panelVisibility, BUSINESS_SECTION_CALENDAR, "Calendar / shift");
    char calendarLine[64];
    formatCalendarDateLabel(calendarStore, calendarLine, sizeof(calendarLine));
    ImGui::Text("Calendar: %s", calendarLine);
    ImGui::Text("Hour: %02d:00 | Week hours: %d / %d", calendarStore.hourOfDay, calendarStore.hoursWorkedThisWeek, calendarStore.scheduledHoursThisWeek);
    if (isPlayerEmployed(playerOperationsStore)) {
        ImGui::Text("Shift: %d:00-%d:00", workScheduleStore.shiftStartHour, workScheduleStore.shiftEndHour);
    }
}

void renderBusinessEmploymentSection(const GameUiFrameContext& frame) {
    PlayerOperationsStore& playerOperationsStore = frame.playerOperationsStore;
    renderBusinessSectionHeader(frame.panelVisibility, BUSINESS_SECTION_EMPLOYMENT, "Current employment");
    if (!isPlayerEmployed(playerOperationsStore)) {
        ImGui::TextDisabled("Unemployed.");
        return;
    }
    contextHelpSectionHeader("Current employment", "Your active jobs.", "employment_status", frame.contextHelpState);
    for (int i = 0; i < 2; ++i) {
        if (playerOperationsStore.employedBusinessIndices[i] >= 0) {
            const BusinessNodeDefinition* job = getBusinessNodeDefinition(playerOperationsStore.employedBusinessIndices[i]);
            if (job != nullptr) {
                const char* scheduleLabel = (job->scheduleType == JobScheduleType::FullTime) ? "Full-time" : "Part-time";
                char wageBuffer[32];
                int64_t monthlyWage = computeBusinessMonthlyWageCents(*job);
                if (job->scheduleType == JobScheduleType::PartTime) {
                    monthlyWage = static_cast<int64_t>(static_cast<float>(monthlyWage) * 0.6f);
                }
                formatCashCents(wageBuffer, sizeof(wageBuffer), monthlyWage);
                ImGui::BulletText("%s (%s) — %s/shift", job->mapLabel, scheduleLabel, wageBuffer);
            }
        }
    }
}

void renderBusinessSelectedSection(const GameUiFrameContext& frame) {
    const ChunkStore& chunkStore = frame.chunkStore;
    const PlayerWorldState& playerWorldState = frame.playerWorldState;
    const ViewportPickState& viewportPickState = frame.viewportPickState;
    renderBusinessSectionHeader(frame.panelVisibility, BUSINESS_SECTION_SELECTED, "Selected business");
    if (!viewportPickState.hasBusinessSelection || viewportPickState.selectedBusinessIndex < 0) {
        ImGui::TextDisabled("Click a blue business node on the map.");
        return;
    }
    const BusinessNodeDefinition* business = getBusinessNodeDefinition(viewportPickState.selectedBusinessIndex);
    if (business == nullptr) {
        ImGui::TextDisabled("Invalid business selection.");
        return;
    }
    ImGui::Text("%s", business->fullName);
    ImGui::Text("Industry: %s", businessIndustryToLabel(business->industry));
    ImGui::TextWrapped("Traits: %s", businessTraitsToShortLabel(business->traitFlags));
    ImGui::Text("Tile: (%d, %d)", business->tileX, business->tileY);
    const RegionId businessRegion = getBusinessNodeRegionId(viewportPickState.selectedBusinessIndex, chunkStore);
    const bool isInRegion = canPlayerOperateInRegion(playerWorldState, businessRegion);
    if (!isInRegion) {
        ImGui::TextColored(ImVec4(0.95f, 0.55f, 0.35f, 1.0f), "You must be in %s to apply.", RegionTable::getRegionShortName(businessRegion).data());
    }
}

void renderBusinessApplySection(
    const GameUiFrameContext& frame,
    GamePanelVisibility& panelVisibility,
    GameModalState& gameModalState) {
    PlayerOperationsStore& playerOperationsStore = frame.playerOperationsStore;
    const PlayerProfile& playerProfile = frame.playerProfile;
    const PlayerWorldState& playerWorldState = frame.playerWorldState;
    const ChunkStore& chunkStore = frame.chunkStore;
    const ViewportPickState& viewportPickState = frame.viewportPickState;
    const uint64_t tickCount = frame.tickCount;
    const uint64_t worldSeed = frame.worldSeed;
    SimClock& simClock = frame.simClock;
    renderBusinessSectionHeader(panelVisibility, BUSINESS_SECTION_APPLY, "Apply / eligibility");
    if (!viewportPickState.hasBusinessSelection || viewportPickState.selectedBusinessIndex < 0) {
        return;
    }
    const BusinessNodeDefinition* business = getBusinessNodeDefinition(viewportPickState.selectedBusinessIndex);
    if (business == nullptr || isLawOfficeBusinessIndex(viewportPickState.selectedBusinessIndex)) {
        return;
    }
    const RegionId businessRegion = getBusinessNodeRegionId(viewportPickState.selectedBusinessIndex, chunkStore);
    const bool isInRegion = canPlayerOperateInRegion(playerWorldState, businessRegion);
    const char* lockReason = nullptr;
    const bool isEligible = evaluateJobEligibility(
        playerProfile,
        playerOperationsStore,
        viewportPickState.selectedBusinessIndex,
        playerOperationsStore.workExperienceMonths,
        tickCount,
        lockReason);
    const uint64_t reapplyTicksRemaining = getJobReapplyTicksRemaining(
        playerOperationsStore,
        viewportPickState.selectedBusinessIndex,
        tickCount);
    if (ImGui::Button("Apply for job")) {
        beginJobInterviewModal(gameModalState, viewportPickState.selectedBusinessIndex, simClock, worldSeed);
    }
    if (!isEligible && lockReason != nullptr) {
        ImGui::TextDisabled("%s", lockReason);
    }
    if (reapplyTicksRemaining > 0ULL) {
        ImGui::TextColored(ImVec4(0.95f, 0.55f, 0.35f, 1.0f), "Reapply cooldown: %llu ticks", static_cast<unsigned long long>(reapplyTicksRemaining));
    }
    if (!isInRegion) {
        ImGui::TextDisabled("Move to the right borough to apply.");
    }
}

void renderBusinessStatusSection(const GameUiFrameContext& frame) {
    renderBusinessSectionHeader(frame.panelVisibility, BUSINESS_SECTION_STATUS, "Status");
    if (isPlayerEmployed(frame.playerOperationsStore)) {
        ImGui::TextDisabled("Currently employed.");
    } else {
        ImGui::TextDisabled("Unemployed.");
    }
}

void renderBusinessSectionById(
    int32_t sectionId,
    const GameUiFrameContext& frame,
    GameModalState& gameModalState) {
    switch (sectionId) {
    case BUSINESS_SECTION_CALENDAR:
        renderBusinessCalendarSection(frame);
        break;
    case BUSINESS_SECTION_EMPLOYMENT:
        renderBusinessEmploymentSection(frame);
        break;
    case BUSINESS_SECTION_SELECTED:
        renderBusinessSelectedSection(frame);
        break;
    case BUSINESS_SECTION_APPLY:
        renderBusinessApplySection(frame, frame.panelVisibility, gameModalState);
        break;
    case BUSINESS_SECTION_STATUS:
        renderBusinessStatusSection(frame);
        break;
    default:
        break;
    }
}

void renderBusinessPanelBody(const GameUiFrameContext& frame, GameModalState& gameModalState) {
    const GamePanelVisibility& panelVisibility = frame.panelVisibility;
    for (int32_t orderIndex = 0; orderIndex < BUSINESS_PANEL_SECTION_COUNT; ++orderIndex) {
        renderBusinessSectionById(panelVisibility.businessSectionOrder[orderIndex], frame, gameModalState);
    }
}

int32_t findContactsSectionIndex(const GamePanelVisibility& panelVisibility, int32_t sectionId) {
    for (int32_t index = 0; index < CONTACTS_PANEL_SECTION_COUNT; ++index) {
        if (panelVisibility.contactsSectionOrder[index] == sectionId) {
            return index;
        }
    }
    return -1;
}

void renderContactsSectionHeader(GamePanelVisibility& panelVisibility, int32_t sectionId, const char* label) {
    const int32_t displayIndex = findContactsSectionIndex(panelVisibility, sectionId);
    ImGui::Separator();
    ImGui::TextUnformatted(label);
    if (displayIndex >= 0) {
        ImGui::SameLine();
        ImGui::PushID(label);
        if (ImGui::SmallButton("↑") && displayIndex > 0) {
            swapInt32(panelVisibility.contactsSectionOrder[displayIndex], panelVisibility.contactsSectionOrder[displayIndex - 1]);
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("↓") && displayIndex + 1 < CONTACTS_PANEL_SECTION_COUNT) {
            swapInt32(panelVisibility.contactsSectionOrder[displayIndex], panelVisibility.contactsSectionOrder[displayIndex + 1]);
        }
        ImGui::PopID();
    }
}

void renderContactHeaderSection(
    const CharacterAgentStore& characterAgentStore,
    const CharacterAgentState& state,
    const char* displayName,
    const char* roleLabel,
    GamePanelVisibility& panelVisibility) {
    renderContactsSectionHeader(panelVisibility, CONTACTS_SECTION_HEADER, displayName);
    ImGui::Text("%s", roleLabel);
    ImGui::Text("Opinion: %d | Trust: %d | Respect: %d", state.opinionOfPlayer, state.trust, state.respect);
    ImGui::Text("Loyalty score: %d", computeAgentLoyaltyScore(state));
    if (hasAgentRelationEvent(state, AgentRelationEventFlags::BetrayedPlayer)) {
        ImGui::TextColored(ImVec4(0.95f, 0.45f, 0.35f, 1.0f), "Flag: betrayed you");
    }
    if (hasAgentRelationEvent(state, AgentRelationEventFlags::SnitchedToPolice)) {
        ImGui::TextColored(ImVec4(0.95f, 0.45f, 0.35f, 1.0f), "Flag: snitched");
    }
    if (hasAgentRelationEvent(state, AgentRelationEventFlags::MarkedRival)) {
        ImGui::TextColored(ImVec4(0.95f, 0.65f, 0.35f, 1.0f), "Flag: marked rival");
    }
}

void renderContactRapportSection(
    const GameUiFrameContext& frame,
    int32_t agentIndex,
    const CharacterAgentState& state,
    GamePanelVisibility& panelVisibility) {
    renderContactsSectionHeader(panelVisibility, CONTACTS_SECTION_ACTIONS, "Build rapport");
    for (int32_t actionIndex = 0; actionIndex < getSocialActionCount(); ++actionIndex) {
        const SocialActionDefinition* action = getSocialActionDefinition(actionIndex);
        if (action == nullptr) {
            continue;
        }
        const SocialActionLockReason lockReason = evaluateSocialActionLock(
            frame.playerSocialActionStore,
            frame.playerCriminalJusticeStore,
            frame.playerWallet,
            frame.playerWorldState,
            frame.playerProfile,
            frame.characterAgentStore,
            frame.chunkStore,
            agentIndex,
            actionIndex,
            frame.tickCount);
        ImGui::PushID(actionIndex + 6000);
        if (lockReason != SocialActionLockReason::None) {
            ImGui::BeginDisabled();
        }
        char actionButtonLabel[64]{};
        if (action->cashCostCents > 0) {
            std::snprintf(actionButtonLabel, sizeof(actionButtonLabel), "%s ($%.2f)", action->displayName, static_cast<double>(action->cashCostCents) / 100.0);
        } else {
            std::snprintf(actionButtonLabel, sizeof(actionButtonLabel), "%s", action->displayName);
        }
        if (ImGui::Button(actionButtonLabel)) {
            tryExecuteSocialAction(
                frame.playerSocialActionStore,
                frame.playerWallet,
                frame.characterAgentStore,
                frame.playerCriminalJusticeStore,
                frame.playerWorldState,
                frame.playerProfile,
                frame.chunkStore,
                agentIndex,
                actionIndex,
                frame.tickCount);
        }
        if (lockReason != SocialActionLockReason::None) {
            ImGui::EndDisabled();
            ImGui::SameLine();
            ImGui::TextDisabled("%s", socialActionLockReasonToString(lockReason));
        }
        ImGui::PopID();
    }
}

void renderContactPressureSection(
    const GameUiFrameContext& frame,
    GameModalState& gameModalState,
    int32_t agentIndex,
    GamePanelVisibility& panelVisibility) {
    renderContactsSectionHeader(panelVisibility, CONTACTS_SECTION_PRESSURE, "Pressure");
    if (agentIndex == BEAT_COP_AGENT_SLOT_INDEX) {
        if (ImGui::Button("Bribe")) {
            beginCovertActionModal(
                gameModalState,
                CovertActionKind::BribePolice,
                agentIndex,
                frame.characterAgentStore,
                frame.playerOrganizationStore,
                frame.playerLawEnforcementStore,
                frame.gameplayStores.lawIntelStore,
                frame.simClock);
        }
    } else if (ImGui::Button("Kidnap")) {
        beginCovertActionModal(
            gameModalState,
            CovertActionKind::KidnapTarget,
            agentIndex,
            frame.characterAgentStore,
            frame.playerOrganizationStore,
            frame.playerLawEnforcementStore,
            frame.gameplayStores.lawIntelStore,
            frame.simClock);
    }
}

void renderContactCrewSection(
    const GameUiFrameContext& frame,
    GameModalState& gameModalState,
    int32_t agentIndex,
    GamePanelVisibility& panelVisibility) {
    renderContactsSectionHeader(panelVisibility, CONTACTS_SECTION_CREW, "Crew");
    if (agentIndex != BEAT_COP_AGENT_SLOT_INDEX) {
        if (ImGui::Button("Assassinate")) {
            beginCovertActionModal(
                gameModalState,
                CovertActionKind::AssassinateTarget,
                agentIndex,
                frame.characterAgentStore,
                frame.playerOrganizationStore,
                frame.playerLawEnforcementStore,
                frame.gameplayStores.lawIntelStore,
                frame.simClock);
        }
    }
    if (frame.playerOrganizationStore.powerTier != PlayerPowerTier::Organization) {
        const CrewRecruitLockReason recruitLock = evaluateCrewRecruitLock(frame.playerOrganizationStore, frame.characterAgentStore, agentIndex);
        if (recruitLock != CrewRecruitLockReason::None) {
            ImGui::BeginDisabled();
        }
        if (ImGui::Button("Talk: recruit to crew")) {
            beginCrewRecruitmentModal(gameModalState, agentIndex, frame.simClock);
        }
        if (recruitLock != CrewRecruitLockReason::None) {
            ImGui::EndDisabled();
            ImGui::TextDisabled("%s", crewRecruitLockReasonToString(recruitLock));
        }
    }
}

void renderContactsPanelBody(const GameUiFrameContext& frame, GameModalState& gameModalState) {
    const PlayerWorkScheduleStore& workScheduleStore = frame.gameplayStores.workScheduleStore;
    const PlayerWorldState& playerWorldState = frame.playerWorldState;
    const CharacterAgentStore& characterAgentStore = frame.characterAgentStore;
    GamePanelVisibility& panelVisibility = frame.panelVisibility;
    if (isPlayerUiRestrictedAtWork(workScheduleStore, playerWorldState)) {
        ImGui::TextDisabled("On shift: approaches and intel contacts can still reach you here.");
    }
    for (int32_t agentIndex = 0; agentIndex < MAX_CHARACTER_AGENT_COUNT; ++agentIndex) {
        if (agentIndex >= NPC_POPULATION_FIRST_SLOT_INDEX) {
            break;
        }
        const CharacterAgentState* state = getCharacterAgentState(characterAgentStore, agentIndex);
        const char* displayName = nullptr;
        const char* roleLabel = nullptr;
        if (state == nullptr || !tryGetAgentDisplayLabels(characterAgentStore, agentIndex, displayName, roleLabel)) {
            continue;
        }
        ImGui::PushID(agentIndex + 5000);
        const int32_t sectionOrder[CONTACTS_PANEL_SECTION_COUNT] = {
            CONTACTS_SECTION_HEADER,
            CONTACTS_SECTION_ACTIONS,
            CONTACTS_SECTION_PRESSURE,
            CONTACTS_SECTION_CREW,
            CONTACTS_SECTION_INLINE,
            CONTACTS_SECTION_TAIL,
        };
        for (int32_t orderIndex = 0; orderIndex < CONTACTS_PANEL_SECTION_COUNT; ++orderIndex) {
            switch (sectionOrder[orderIndex]) {
            case CONTACTS_SECTION_HEADER:
                renderContactHeaderSection(characterAgentStore, *state, displayName, roleLabel, panelVisibility);
                break;
            case CONTACTS_SECTION_ACTIONS:
                renderContactRapportSection(frame, agentIndex, *state, panelVisibility);
                break;
            case CONTACTS_SECTION_PRESSURE:
                renderContactPressureSection(frame, gameModalState, agentIndex, panelVisibility);
                break;
            case CONTACTS_SECTION_CREW:
                renderContactCrewSection(frame, gameModalState, agentIndex, panelVisibility);
                break;
            case CONTACTS_SECTION_INLINE:
            case CONTACTS_SECTION_TAIL:
            default:
                break;
            }
        }
        ImGui::PopID();
    }
}

} // namespace

void renderOperationsPanel(const GameUiFrameContext& frame, GameModalState& gameModalState) {
    const WorldConfig& worldConfig = frame.worldConfig;
    const PlayerWorkScheduleStore& workScheduleStore = frame.gameplayStores.workScheduleStore;
    const ChunkStore& chunkStore = frame.chunkStore;
    PlayerOperationsStore& playerOperationsStore = frame.playerOperationsStore;
    PlayerOrganizationStore& playerOrganizationStore = frame.playerOrganizationStore;
    PlayerStreetCrimeStore& playerStreetCrimeStore = frame.playerStreetCrimeStore;
    PlayerLawEnforcementStore& playerLawEnforcementStore = frame.playerLawEnforcementStore;
    PlayerLawIntelStore& playerLawIntelStore = frame.gameplayStores.lawIntelStore;
    const PlayerInformationFeedStore& informationFeedStore = frame.gameplayStores.informationFeedStore;
    PlayerCriminalJusticeStore& playerCriminalJusticeStore = frame.playerCriminalJusticeStore;
    PlayerLegalCounselStore& legalCounselStore = frame.gameplayStores.legalCounselStore;
    const PlayerNarrativeArchiveStore& narrativeArchiveStore = frame.gameplayStores.narrativeArchiveStore;
    PlayerWallet& playerWallet = frame.playerWallet;
    PlayerWorldState& playerWorldState = frame.playerWorldState;
    CharacterAgentStore& characterAgentStore = frame.characterAgentStore;
    SimEventQueue& simEventQueue = frame.simEventQueue;
    const PlayerProfile& playerProfile = frame.playerProfile;
    SimClock& simClock = frame.simClock;
    const uint64_t tickCount = frame.tickCount;
    const ViewportPickState& viewportPickState = frame.viewportPickState;
    GamePanelVisibility& panelVisibility = frame.panelVisibility;
    ContextHelpState& contextHelpState = frame.contextHelpState;
    if (!panelVisibility.showOperations) {
        return;
    }
    ImGui::SetNextWindowSizeConstraints(ImVec2(300.0f, 260.0f), ImVec2(FLT_MAX, FLT_MAX));
    bool isOpen = true;
    const bool isDockPanelVisible = beginDockPanelWindow(GameDockPanel::Operations, &isOpen);
    panelVisibility.showOperations = isOpen;
    if (isDockPanelVisible) {
        contextHelpPanelTag("Operations Panel", "Establish headquarters, rackets, and fronts.", "operations_panel", contextHelpState);
        const bool isAtWorkRestricted = isPlayerUiRestrictedAtWork(workScheduleStore, playerWorldState);
        renderOperationsPanelBody(frame, gameModalState, isAtWorkRestricted);
    }
    ImGui::End();
}

void renderBusinessPanel(const GameUiFrameContext& frame, GameModalState& gameModalState) {
    const WorldConfig& worldConfig = frame.worldConfig;
    const ChunkStore& chunkStore = frame.chunkStore;
    PlayerOperationsStore& playerOperationsStore = frame.playerOperationsStore;
    PlayerWallet& playerWallet = frame.playerWallet;
    const PlayerProfile& playerProfile = frame.playerProfile;
    const PlayerWorldState& playerWorldState = frame.playerWorldState;
    const GameCalendarStore& calendarStore = frame.gameplayStores.calendarStore;
    const PlayerWorkScheduleStore& workScheduleStore = frame.gameplayStores.workScheduleStore;
    SimClock& simClock = frame.simClock;
    const ViewportPickState& viewportPickState = frame.viewportPickState;
    const uint64_t tickCount = frame.tickCount;
    const uint64_t worldSeed = frame.worldSeed;
    GamePanelVisibility& panelVisibility = frame.panelVisibility;
    ContextHelpState& contextHelpState = frame.contextHelpState;
    if (!panelVisibility.showBusiness) {
        return;
    }
    ImGui::SetNextWindowSizeConstraints(ImVec2(300.0f, 220.0f), ImVec2(FLT_MAX, FLT_MAX));
    bool isOpen = true;
    const bool isDockPanelVisible = beginDockPanelWindow(GameDockPanel::Business, &isOpen);
    panelVisibility.showBusiness = isOpen;
    if (isDockPanelVisible) {
        contextHelpPanelTag("Business Panel", "Blue map nodes: jobs and borough trade.", "business_panel", contextHelpState);
        renderBusinessPanelBody(frame, gameModalState);
    }
    ImGui::End();
}

void renderContactsPanel(const GameUiFrameContext& frame, GameModalState& gameModalState) {
    const PlayerWorkScheduleStore& workScheduleStore = frame.gameplayStores.workScheduleStore;
    const PlayerWorldState& playerWorldState = frame.playerWorldState;
    const CharacterAgentStore& characterAgentStore = frame.characterAgentStore;
    PlayerSocialActionStore& playerSocialActionStore = frame.playerSocialActionStore;
    const PlayerOrganizationStore& playerOrganizationStore = frame.playerOrganizationStore;
    const PlayerLawEnforcementStore& playerLawEnforcementStore = frame.playerLawEnforcementStore;
    const PlayerLawIntelStore& playerLawIntelStore = frame.gameplayStores.lawIntelStore;
    const PlayerCriminalJusticeStore& playerCriminalJusticeStore = frame.playerCriminalJusticeStore;
    const PlayerProfile& playerProfile = frame.playerProfile;
    PlayerWallet& playerWallet = frame.playerWallet;
    ChunkStore& chunkStore = frame.chunkStore;
    const uint64_t tickCount = frame.tickCount;
    SimClock& simClock = frame.simClock;
    GamePanelVisibility& panelVisibility = frame.panelVisibility;
    ContextHelpState& contextHelpState = frame.contextHelpState;
    if (!panelVisibility.showContacts) {
        return;
    }
    ImGui::SetNextWindowSizeConstraints(ImVec2(280.0f, 240.0f), ImVec2(FLT_MAX, FLT_MAX));
    bool isOpen = true;
    const bool isDockPanelVisible = beginDockPanelWindow(GameDockPanel::Contacts, &isOpen);
    panelVisibility.showContacts = isOpen;
    if (isDockPanelVisible) {
        contextHelpPanelTag("Contacts Panel", "AI characters and their view of you.", "contacts_panel", contextHelpState);
        renderContactsPanelBody(frame, gameModalState);
    }
    ImGui::End();
}

} // namespace Core
