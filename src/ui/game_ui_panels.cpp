#include "ui/game_ui_panels.h"
#include "ui/game_dock_panels.h"
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
#include "game/job_catalog.h"
#include "game/legal_counsel.h"
#include "game/player_law_intel.h"
#include "game/player_information_feed.h"
#include "game/agent_relation_events.h"
#include "game/travel_modes.h"
#include "game/game_calendar.h"
#include "game/player_work_schedule.h"
#include "game/player_employment.h"
#include "ui/dock_panel_helpers.h"
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
    const WorldEventStore& worldEventStore = frame.worldEventStore;
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
    if (isAtWorkRestricted) {
        ImGui::TextColored(ImVec4(0.55f, 0.85f, 1.0f, 1.0f), "On shift — limited panel access. Simulation keeps running.");
        ImGui::TextDisabled("Street ops and travel are locked. Contacts and map events can still reach you.");
        ImGui::Separator();
    }
    if (!hasPlayerHeadquarters(playerOperationsStore)) {
        ImGui::TextWrapped("Claim a rented room or apartment as your headquarters — your first home is your base of operations.");
        ImGui::TextDisabled("Move-in deposit is paid once; rent, taxes, and utilities bill monthly. Family stay (DPA) is optional if you have kin in-country.");
        ImGui::Separator();
    } else {
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
        if (operation->category != OperationCategory::Headquarters && isOperationCatalogActive(playerOperationsStore, catalogIndex)) {
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
        "Heat: %d | %s",
        playerLawEnforcementStore.personalHeat,
        getPoliceInvestigationLabel(playerLawEnforcementStore.investigationTier));
    if (doesPlayerKnowAboutWarrants(playerLawIntelStore, playerLawEnforcementStore)) {
        ImGui::Text("Known warrants: %d", getPlayerKnownWarrantCount(playerLawIntelStore, playerLawEnforcementStore));
    }
    if (playerLawEnforcementStore.witnessCount > 0) {
        ImGui::TextDisabled("Witnesses on record: %d (%s)", playerLawEnforcementStore.witnessCount, playerLawEnforcementStore.lastWitnessLabel);
    }
    ImGui::Text("Criminal trust (best): %d", computeBestCriminalContactTrust(characterAgentStore));
    if (playerWallet.cashCents <= STREET_CRIME_BROKE_CASH_THRESHOLD_CENTS) {
        ImGui::TextColored(ImVec4(0.95f, 0.75f, 0.35f, 1.0f), "Low cash — solo crimes are your fastest option.");
    }
    if (!hasPlayerHeadquarters(playerOperationsStore) && !isAtWorkRestricted) {
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
    } else if (!isAtWorkRestricted) {
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
    ImGui::Separator();
    contextHelpSectionHeader("Information", "Newspapers, rumors, and network intel.", "law_intel", contextHelpState);
    ImGui::Text("Unread items: %d", getUnreadInformationFeedCount(informationFeedStore));
    ImGui::TextDisabled("Map alerts appear bottom-right; newspapers pause sim when opened.");
    ImGui::Separator();
    contextHelpSectionHeader("Narrative archive", "Logged motives for headlines and former-life collectibles.", "law_intel", contextHelpState);
    ImGui::Text("Story beats recorded: %d", getNarrativeBeatCount(narrativeArchiveStore));
    const int32_t beatCount = getNarrativeBeatCount(narrativeArchiveStore);
    if (beatCount > 0) {
        ImGui::TextDisabled("Latest: %s", narrativeArchiveStore.beats[beatCount - 1].headline);
    }
    ImGui::Separator();
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
    if (!isAtWorkRestricted) {
        ImGui::Separator();
        contextHelpSectionHeader("Travel", "Move between boroughs with realistic time costs.", "travel_modes", contextHelpState);
        ImGui::Text("Location: tile (%d, %d)", playerWorldState.currentTileX, playerWorldState.currentTileY);
        if (playerWorldState.isTraveling) {
            ImGui::TextDisabled("In transit...");
        }
        static int32_t travelTargetX = 240;
        static int32_t travelTargetY = 240;
        static int32_t travelModeIndex = 0;
        if (viewportPickState.hasSelection) {
            travelTargetX = viewportPickState.selectedCoord.x;
            travelTargetY = viewportPickState.selectedCoord.y;
        }
        ImGui::InputInt("Target X", &travelTargetX);
        ImGui::InputInt("Target Y", &travelTargetY);
        if (viewportPickState.hasSelection) {
            ImGui::TextDisabled("Target from map click: (%d, %d)", travelTargetX, travelTargetY);
        }
        ImGui::Combo("Mode", &travelModeIndex, "Walk\0Bicycle\0Car\0Train\0");
        if (ImGui::Button("Plan / execute travel")) {
            TravelPlan plan{};
            const TravelMode mode = static_cast<TravelMode>(travelModeIndex);
            buildTravelPlan(
                plan,
                mode,
                playerWorldState.currentTileX,
                playerWorldState.currentTileY,
                travelTargetX,
                travelTargetY,
                chunkStore,
                worldConfig);
            const WorldCoord targetCoord{travelTargetX, travelTargetY};
            const RegionId targetRegion = chunkStore.getRegionAt(targetCoord);
            tryExecuteTravelPlan(plan, playerWorldState, targetRegion, tickCount, playerWallet, chunkStore, worldConfig);
        }
    }
    }
    ImGui::End();
}

void renderBusinessPanel(const GameUiFrameContext& frame, GameModalState& gameModalState) {
    const WorldConfig& worldConfig = frame.worldConfig;
    const ChunkStore& chunkStore = frame.chunkStore;
    PlayerOperationsStore& playerOperationsStore = frame.playerOperationsStore;
    PlayerWallet& playerWallet = frame.playerWallet;
    SimEventQueue& simEventQueue = frame.simEventQueue;
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
    contextHelpSectionHeader("Calendar & shift", "Date, hours, and work schedule.", "travel_schedule", contextHelpState);
    char calendarLine[64];
    formatCalendarDateLabel(calendarStore, calendarLine, sizeof(calendarLine));
    ImGui::Text("Calendar: %s", calendarLine);
    ImGui::Text("Hour: %02d:00 | Week hours: %d / %d", calendarStore.hourOfDay, calendarStore.hoursWorkedThisWeek, calendarStore.scheduledHoursThisWeek);
    if (isPlayerEmployed(playerOperationsStore)) {
        ImGui::Text("Shift: %d:00-%d:00", workScheduleStore.shiftStartHour, workScheduleStore.shiftEndHour);
        ImGui::Separator();
        contextHelpSectionHeader("Current employment", "Your active jobs.", "employment_status", contextHelpState);
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
        int32_t jobCount = getPlayerJobCount(playerOperationsStore);
        if (jobCount < 2) {
            if (hasFullTimeJob(playerOperationsStore)) {
                ImGui::TextDisabled("[Can accept 1 part-time job]");
            } else if (jobCount == 0) {
                ImGui::TextDisabled("[Can accept 2 part-time or 1 full-time job]");
            } else {
                ImGui::TextDisabled("[Can accept 1 more part-time job]");
            }
        }
    }
    ImGui::Separator();
    if (!viewportPickState.hasBusinessSelection || viewportPickState.selectedBusinessIndex < 0) {
        ImGui::TextDisabled("Click a blue business node on the map.");
    } else {
        const BusinessNodeDefinition* business = getBusinessNodeDefinition(viewportPickState.selectedBusinessIndex);
        if (business == nullptr) {
            ImGui::TextDisabled("Invalid business selection.");
        } else {
            ImGui::Text("%s", business->fullName);
            ImGui::Text("Industry: %s", businessIndustryToLabel(business->industry));
            ImGui::TextWrapped("Traits: %s", businessTraitsToShortLabel(business->traitFlags));
            ImGui::Text("Tile: (%d, %d)", business->tileX, business->tileY);
            const RegionId businessRegion = getBusinessNodeRegionId(viewportPickState.selectedBusinessIndex, chunkStore);
            (void)chunkStore;
            const bool isInRegion = canPlayerOperateInRegion(playerWorldState, businessRegion);
            if (isLawOfficeBusinessIndex(viewportPickState.selectedBusinessIndex)) {
                if (!isInRegion) {
                    ImGui::TextColored(ImVec4(0.95f, 0.55f, 0.35f, 1.0f), "Visit this office in person to retain counsel.");
                } else {
                    ImGui::TextWrapped("Law office — retain counsel from Operations while a case is active.");
                }
            } else {
                char wageBuffer[32];
                formatCashCents(wageBuffer, sizeof(wageBuffer), computeBusinessMonthlyWageCents(*business));
                ImGui::Text("Monthly wage (est.): %s", wageBuffer);
                const JobDefinitionExtension* extension = getJobDefinitionExtension(viewportPickState.selectedBusinessIndex);
                if (extension != nullptr) {
                    ImGui::Text("Perks: %s", jobPerkFlagsToShortLabel(extension->perkFlags));
                    if (extension->minWorkExperienceMonths > 0) {
                        ImGui::Text("Experience required: %d months", extension->minWorkExperienceMonths);
                    }
                }
                if (!isInRegion) {
                    ImGui::TextColored(ImVec4(0.95f, 0.55f, 0.35f, 1.0f), "You must be in %s to apply.", RegionTable::getRegionShortName(businessRegion).data());
                }
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
                if (reapplyTicksRemaining > 0ULL) {
                    ImGui::TextColored(ImVec4(0.95f, 0.55f, 0.35f, 1.0f), "Reapply cooldown: %llu ticks", static_cast<unsigned long long>(reapplyTicksRemaining));
                }
                if (getNetworkAccessScore(playerProfile) < business->minNetworkAccess) {
                    ImGui::BeginDisabled();
                }
                if (!isInRegion || !canAcceptJob(playerOperationsStore, business->scheduleType) || !isEligible || reapplyTicksRemaining > 0ULL) {
                    ImGui::BeginDisabled();
                }
                if (ImGui::Button("Apply for job")) {
                    beginJobInterviewModal(gameModalState, viewportPickState.selectedBusinessIndex, simClock, worldSeed);
                }
                if (!isInRegion || !canAcceptJob(playerOperationsStore, business->scheduleType) || !isEligible || reapplyTicksRemaining > 0ULL) {
                    ImGui::EndDisabled();
                }
                if (!isEligible && lockReason != nullptr) {
                    ImGui::TextDisabled("%s", lockReason);
                }
                if (getNetworkAccessScore(playerProfile) < business->minNetworkAccess) {
                    ImGui::EndDisabled();
                    ImGui::TextDisabled("Requires higher network access.");
                }
                if (isPlayerEmployed(playerOperationsStore)) {
                    bool isEmployedHere = false;
                    for (int i = 0; i < 2; ++i) {
                        if (playerOperationsStore.employedBusinessIndices[i] == viewportPickState.selectedBusinessIndex) {
                            isEmployedHere = true;
                            break;
                        }
                    }
                    if (isEmployedHere) {
                        ImGui::Text("Status: Employed here");
                    }
                }
                if (playerWorldState.isAtWork) {
                    ImGui::TextDisabled("Currently on shift — limited actions.");
                }
            }
        }
    }
    }
    ImGui::End();
}

void renderContactsPanel(const GameUiFrameContext& frame, GameModalState& gameModalState) {
    const PlayerWorkScheduleStore& workScheduleStore = frame.gameplayStores.workScheduleStore;
    const PlayerWorldState& playerWorldState = frame.playerWorldState;
    const CharacterAgentStore& characterAgentStore = frame.characterAgentStore;
    const PlayerOrganizationStore& playerOrganizationStore = frame.playerOrganizationStore;
    const PlayerLawEnforcementStore& playerLawEnforcementStore = frame.playerLawEnforcementStore;
    const PlayerLawIntelStore& playerLawIntelStore = frame.gameplayStores.lawIntelStore;
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
    if (isPlayerUiRestrictedAtWork(workScheduleStore, playerWorldState)) {
        ImGui::TextDisabled("On shift: approaches and intel contacts can still reach you here.");
        ImGui::Separator();
    }
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
        ImGui::TextDisabled("Build relationships through covert actions, favors, and avoiding betrayal.");
        if (hasAgentRelationEvent(*state, AgentRelationEventFlags::BetrayedPlayer)) {
            ImGui::TextColored(ImVec4(0.95f, 0.45f, 0.35f, 1.0f), "Flag: betrayed you");
        }
        if (hasAgentRelationEvent(*state, AgentRelationEventFlags::SnitchedToPolice)) {
            ImGui::TextColored(ImVec4(0.95f, 0.45f, 0.35f, 1.0f), "Flag: snitched");
        }
        ImGui::PushID(agentIndex + 5000);
        if (agentIndex == BEAT_COP_AGENT_SLOT_INDEX) {
            if (ImGui::Button("Bribe")) {
                beginCovertActionModal(
                    gameModalState,
                    CovertActionKind::BribePolice,
                    agentIndex,
                    characterAgentStore,
                    playerOrganizationStore,
                    playerLawEnforcementStore,
                    playerLawIntelStore,
                    simClock);
            }
        } else {
            if (ImGui::Button("Kidnap")) {
                beginCovertActionModal(
                    gameModalState,
                    CovertActionKind::KidnapTarget,
                    agentIndex,
                    characterAgentStore,
                    playerOrganizationStore,
                    playerLawEnforcementStore,
                    playerLawIntelStore,
                    simClock);
            }
            ImGui::SameLine();
            if (ImGui::Button("Assassinate")) {
                beginCovertActionModal(
                    gameModalState,
                    CovertActionKind::AssassinateTarget,
                    agentIndex,
                    characterAgentStore,
                    playerOrganizationStore,
                    playerLawEnforcementStore,
                    playerLawIntelStore,
                    simClock);
            }
        }
        ImGui::PopID();
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
    }
    ImGui::End();
}

} // namespace Core
