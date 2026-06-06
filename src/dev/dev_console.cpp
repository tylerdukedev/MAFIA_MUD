#include "dev/dev_console.h"
#include "game/crime_legal_tier.h"
#include "game/player_criminal_justice.h"
#include "game/player_law_enforcement.h"
#include "game/player_organization.h"
#include "character/character_family.h"
#include "character/character_tables.h"
#include "character/profile_builder.h"
#include "game/economy_constants.h"
#include "game/player_operations.h"
#include "sim/world_event_catalog.h"
#include "sim/world_event_system.h"
#include "character/character_social_network.h"
#include "sim/world_event_store.h"
#include <cstdlib>
#include "game/player_wallet.h"
#include "game/game_calendar.h"
#include "game/player_law_intel.h"
#include "game/covert_action_executor.h"
#include "game/action_reason_catalog.h"
#include "game/legal_counsel.h"
#include "sim/character_agent.h"
#include "sim/agent_relationship_graph.h"
#include "game/npc_decision.h"
#include "game/property_store.h"
#include "world/business_node_table.h"
#include <algorithm>
#include "world/city_control.h"
#include "world/landmark_table.h"
#include "imgui.h"
#include <cctype>
#include <cstdio>
#include <cstring>

namespace Core {

namespace {
constexpr const char* DEV_CONSOLE_WINDOW_NAME = "Dev Console";

void skipWhitespace(const char*& cursor) {
    while (*cursor != '\0' && std::isspace(static_cast<unsigned char>(*cursor))) {
        ++cursor;
    }
}

bool readToken(const char*& cursor, char* outToken, size_t tokenSize) {
    skipWhitespace(cursor);
    if (*cursor == '\0' || tokenSize == 0) {
        return false;
    }
    size_t writeIndex = 0;
    while (*cursor != '\0' && !std::isspace(static_cast<unsigned char>(*cursor)) && writeIndex + 1 < tokenSize) {
        outToken[writeIndex] = *cursor;
        ++writeIndex;
        ++cursor;
    }
    outToken[writeIndex] = '\0';
    return writeIndex > 0;
}

void logProfileAxis(DevConsoleLog& log, const char* title, const char* lineOne, const char* lineTwo) {
    char buffer[DEV_CONSOLE_LOG_LINE_SIZE];
    std::snprintf(buffer, sizeof(buffer), "%s", title);
    devConsoleLogAppend(log, buffer);
    devConsoleLogAppend(log, lineOne);
    if (lineTwo[0] != '\0') {
        devConsoleLogAppend(log, lineTwo);
    }
}

void logDraftFields(DevConsoleLog& log, const CharacterDraft& draft) {
    char buffer[DEV_CONSOLE_LOG_LINE_SIZE];
    std::snprintf(
        buffer,
        sizeof(buffer),
        "Draft: name=%s age=%d nationality=%.*s heritage=%.*s generation=%.*s background=%.*s borough=%.*s",
        draft.nameBuffer,
        draft.age,
        static_cast<int>(getNationalityName(draft.nationalityId).size()),
        getNationalityName(draft.nationalityId).data(),
        static_cast<int>(getHeritageName(draft.heritageId).size()),
        getHeritageName(draft.heritageId).data(),
        static_cast<int>(getGenerationName(draft.generationId).size()),
        getGenerationName(draft.generationId).data(),
        static_cast<int>(getBackgroundName(draft.backgroundId).size()),
        getBackgroundName(draft.backgroundId).data(),
        static_cast<int>(getBoroughPreferenceName(draft.selectedBoroughIndex).size()),
        getBoroughPreferenceName(draft.selectedBoroughIndex).data());
    devConsoleLogAppend(log, buffer);
    const LandmarkDefinition* startingCity = getLandmarkDefinition(draft.startingCityLandmarkIndex);
    char cashBuffer[32];
    formatCashCents(cashBuffer, sizeof(cashBuffer), draft.startingCashCents);
    if (startingCity != nullptr) {
        std::snprintf(
            buffer,
            sizeof(buffer),
            "Start: city=%s (idx %d) cash=%s rollSeed=%llu",
            startingCity->fullName,
            draft.startingCityLandmarkIndex,
            cashBuffer,
            static_cast<unsigned long long>(draft.characterRollSeed));
    } else {
        std::snprintf(
            buffer,
            sizeof(buffer),
            "Start: city=unrolled cash=%s rollSeed=%llu",
            cashBuffer,
            static_cast<unsigned long long>(draft.characterRollSeed));
    }
    devConsoleLogAppend(log, buffer);
    char culturalBuffer[DEV_CONSOLE_LOG_LINE_SIZE];
    formatFamilyCulturalGameplayLines(draft.familyCulturalProfile, culturalBuffer, sizeof(culturalBuffer));
    devConsoleLogAppend(log, culturalBuffer);
    for (int32_t memberIndex = 0; memberIndex < draft.familyMemberCount; ++memberIndex) {
        const FamilyMemberRecord& member = draft.familyMembers[memberIndex];
        std::snprintf(
            buffer,
            sizeof(buffer),
            "Family: %s %s presence=%s",
            member.roleLabel,
            member.displayName,
            getFamilyMemberPresenceLabel(member.presence));
        devConsoleLogAppend(log, buffer);
    }
}

void logWalletFields(DevConsoleLog& log, const PlayerWallet& wallet) {
    char buffer[DEV_CONSOLE_LOG_LINE_SIZE];
    char cashBuffer[32];
    formatCashCents(cashBuffer, sizeof(cashBuffer), wallet.cashCents);
    std::snprintf(buffer, sizeof(buffer), "Wallet: cash=%s legit=%lld crime=%lld", cashBuffer, static_cast<long long>(wallet.lifetimeLegitCents), static_cast<long long>(wallet.lifetimeCrimeCents));
    devConsoleLogAppend(log, buffer);
    std::snprintf(
        buffer,
        sizeof(buffer),
        "Income/tick: legit=%.3f crime=%.3f (paid every %d ticks)",
        wallet.legitIncomePerTickCents,
        wallet.crimeIncomePerTickCents,
        ECONOMY_INCOME_APPLY_INTERVAL_TICKS);
    devConsoleLogAppend(log, buffer);
    std::snprintf(buffer, sizeof(buffer), "Broke threshold: %s", isWalletBroke(wallet) ? "yes" : "no");
    devConsoleLogAppend(log, buffer);
}

void logCityControlFields(DevConsoleLog& log, const CityControlStore& cityControlStore) {
    char buffer[DEV_CONSOLE_LOG_LINE_SIZE];
    const int32_t ownedCount = countPlayerOwnedCities(cityControlStore);
    std::snprintf(buffer, sizeof(buffer), "Cities: player-owned=%d / %d landmarks", ownedCount, getLandmarkCount());
    devConsoleLogAppend(log, buffer);
    char claimBuffer[32];
    formatCashCents(claimBuffer, sizeof(claimBuffer), DEFAULT_CLAIM_CITY_COST_CENTS);
    char hustlerBuffer[32];
    formatCashCents(hustlerBuffer, sizeof(hustlerBuffer), STREET_HUSTLER_CLAIM_COST_CENTS);
    std::snprintf(buffer, sizeof(buffer), "Claim cost: default=%s street_hustler=%s", claimBuffer, hustlerBuffer);
    devConsoleLogAppend(log, buffer);
}

void logProfileDump(DevConsoleLog& log, const CharacterDraft& draft, const PlayerProfile& profile) {
    logDraftFields(log, draft);
    char lineOne[DEV_CONSOLE_LOG_LINE_SIZE];
    char lineTwo[DEV_CONSOLE_LOG_LINE_SIZE];
    std::snprintf(
        lineOne,
        sizeof(lineOne),
        "Network: ethnic=%.2f political=%.2f law=%.2f business=%.2f import=%.2f",
        profile.networkAccess.ethnicNetwork,
        profile.networkAccess.politicalMachine,
        profile.networkAccess.lawEnforcementChannel,
        profile.networkAccess.businessAssociation,
        profile.networkAccess.importPipeline);
    logProfileAxis(log, "[Network Access]", lineOne, "");
    std::snprintf(
        lineOne,
        sizeof(lineOne),
        "Legitimacy: policeDecay=%.2f shellCo=%.2f publicJob=%.2f suspicion=%.2f",
        profile.legitimacy.policeAttentionDecay,
        profile.legitimacy.shellCompanyEase,
        profile.legitimacy.publicFacingJobAccess,
        profile.legitimacy.mainstreamSuspicion);
    logProfileAxis(log, "[Legitimacy]", lineOne, "");
    std::snprintf(
        lineOne,
        sizeof(lineOne),
        "Loyalty: ethnicResist=%.2f kinPref=%.2f detectRisk=%.2f individual=%.2f",
        profile.loyaltyBias.ethnicFactionResistance,
        profile.loyaltyBias.kinAlliancePreference,
        profile.loyaltyBias.mainstreamDetectionRisk,
        profile.loyaltyBias.individualisticLoyalty);
    logProfileAxis(log, "[Loyalty Bias]", lineOne, "");
    std::snprintf(
        lineOne,
        sizeof(lineOne),
        "Culture: inGroup=%.2f outGroup=%.2f penalty=%.2f language=%.2f translate=%.2f",
        profile.culturalCompetency.inGroupNegotiation,
        profile.culturalCompetency.outGroupNegotiation,
        profile.culturalCompetency.crossEthnicPenalty,
        profile.culturalCompetency.languageAccess,
        profile.culturalCompetency.translateBonus);
    logProfileAxis(log, "[Cultural Competency]", lineOne, "");
    std::snprintf(
        lineOne,
        sizeof(lineOne),
        "Paths: street=%.2f organizer=%.2f institutional=%.2f corporate=%.2f",
        profile.opportunityPaths.streetCrimePath,
        profile.opportunityPaths.organizerPath,
        profile.opportunityPaths.institutionalPath,
        profile.opportunityPaths.corporatePath);
    logProfileAxis(log, "[Opportunity Paths]", lineOne, "");
}

void logOperationsFields(DevConsoleLog& log, const PlayerOperationsStore& store) {
    char buffer[DEV_CONSOLE_LOG_LINE_SIZE];
    std::snprintf(buffer, sizeof(buffer), "HQ kind: %d active ops: %d employed business: [%d, %d]", static_cast<int>(store.headquartersKind), store.activeOperationCount, store.employedBusinessIndices[0], store.employedBusinessIndices[1]);
    devConsoleLogAppend(log, buffer);
    std::snprintf(buffer, sizeof(buffer), "HQ tick: %llu monthly ledger tick: %llu family upkeep tick: %llu", static_cast<unsigned long long>(store.headquartersEstablishedTick), static_cast<unsigned long long>(store.lastMonthlyLedgerTick), static_cast<unsigned long long>(store.lastFamilyUpkeepTick));
    devConsoleLogAppend(log, buffer);
}

void logAgentsFields(DevConsoleLog& log, const CharacterAgentStore& store) {
    for (int32_t agentIndex = 0; agentIndex < MAX_CHARACTER_AGENT_COUNT; ++agentIndex) {
        const CharacterAgentState* state = getCharacterAgentState(store, agentIndex);
        const char* displayName = nullptr;
        const char* roleLabel = nullptr;
        if (state == nullptr || !tryGetAgentDisplayLabels(store, agentIndex, displayName, roleLabel)) {
            continue;
        }
        char buffer[DEV_CONSOLE_LOG_LINE_SIZE];
        std::snprintf(buffer, sizeof(buffer), "%s (%s) opinion=%d trust=%d", displayName, roleLabel, state->opinionOfPlayer, state->trust);
        devConsoleLogAppend(log, buffer);
    }
}

void logHelp(DevConsoleLog& log) {
    devConsoleLogAppend(log, "Build: crew/org tiers, police, jail bond/court/probation/parole");
    devConsoleLogAppend(log, "Commands:");
    devConsoleLogAppend(log, "  help");
    devConsoleLogAppend(log, "  log clear");
    devConsoleLogAppend(log, "  profile dump | draft show");
    devConsoleLogAppend(log, "  wallet show | cities show | operations show | crew show | law show | agents show  (in-game)");
    devConsoleLogAppend(log, "  calendar show | counsel show | intel bribe | intel flag <slot> <betrayed|snitch>  (in-game)");
    devConsoleLogAppend(log, "  calendar hours <0-60> | calendar skipday  (in-game)");
    devConsoleLogAppend(log, "  event fire <id> | event flags | agent deactivate <slot>");
    devConsoleLogAppend(log, "  profile set generation <immigrant|first|second|third>");
    devConsoleLogAppend(log, "  profile set heritage <name> | nationality <name> | age <16-25>");
    devConsoleLogAppend(log, "  network show | legitimacy show | loyalty show | culture show | paths show");
}
} // namespace

void devConsoleLogAppend(DevConsoleLog& log, const char* message) {
    if (message == nullptr) {
        return;
    }
    char* targetLine = log.lines[log.writeIndex];
    std::snprintf(targetLine, DEV_CONSOLE_LOG_LINE_SIZE, "%s", message);
    log.writeIndex = (log.writeIndex + 1) % DEV_CONSOLE_LOG_LINE_COUNT;
    if (log.lineCount < DEV_CONSOLE_LOG_LINE_COUNT) {
        ++log.lineCount;
    }
}

void devConsoleLogClear(DevConsoleLog& log) {
    log.writeIndex = 0;
    log.lineCount = 0;
    for (int32_t index = 0; index < DEV_CONSOLE_LOG_LINE_COUNT; ++index) {
        log.lines[index][0] = '\0';
    }
}

void devConsoleToggleVisibility(DevConsoleState& state) {
    state.isVisible = !state.isVisible;
    if (state.isVisible) {
        state.requestFocusInput = true;
    }
}

bool devConsoleParseProfileSetCommand(
    const char* commandLine,
    CharacterDraft& draft,
    PlayerProfile& profile,
    char* outMessage,
    size_t messageSize) {
    if (commandLine == nullptr || outMessage == nullptr || messageSize == 0) {
        return false;
    }
    outMessage[0] = '\0';
    const char* cursor = commandLine;
    char token[64];
    if (!readToken(cursor, token, sizeof(token)) || std::strcmp(token, "profile") != 0) {
        return false;
    }
    if (!readToken(cursor, token, sizeof(token)) || std::strcmp(token, "set") != 0) {
        std::snprintf(outMessage, messageSize, "Expected: profile set <field> <value>");
        return false;
    }
    if (!readToken(cursor, token, sizeof(token))) {
        std::snprintf(outMessage, messageSize, "Missing profile field.");
        return false;
    }
    char valueToken[64];
    if (!readToken(cursor, valueToken, sizeof(valueToken))) {
        std::snprintf(outMessage, messageSize, "Missing profile value.");
        return false;
    }
    if (std::strcmp(token, "generation") == 0) {
        const GenerationId generationId = parseGenerationName(valueToken);
        if (generationId == GenerationId::None) {
            std::snprintf(outMessage, messageSize, "Unknown generation: %s", valueToken);
            return false;
        }
        draft.generationId = generationId;
    } else if (std::strcmp(token, "heritage") == 0) {
        const HeritageId heritageId = parseHeritageName(valueToken);
        if (heritageId == HeritageId::None) {
            std::snprintf(outMessage, messageSize, "Unknown heritage: %s", valueToken);
            return false;
        }
        draft.heritageId = heritageId;
    } else if (std::strcmp(token, "nationality") == 0) {
        const NationalityId nationalityId = parseNationalityName(valueToken);
        if (nationalityId == NationalityId::None) {
            std::snprintf(outMessage, messageSize, "Unknown nationality: %s", valueToken);
            return false;
        }
        draft.nationalityId = nationalityId;
    } else if (std::strcmp(token, "age") == 0) {
        const int32_t ageValue = std::atoi(valueToken);
        if (ageValue < CHARACTER_CREATION_MIN_AGE || ageValue > CHARACTER_CREATION_MAX_AGE) {
            std::snprintf(outMessage, messageSize, "Age must be %d-%d.", CHARACTER_CREATION_MIN_AGE, CHARACTER_CREATION_MAX_AGE);
            return false;
        }
        draft.age = ageValue;
    } else {
        std::snprintf(outMessage, messageSize, "Unknown profile field: %s", token);
        return false;
    }
    profile = buildPlayerProfile(draft);
    std::snprintf(outMessage, messageSize, "Updated profile field %s.", token);
    return true;
}

void devConsoleExecuteCommand(
    DevConsoleLog& log,
    const char* commandLine,
    CharacterDraft& draft,
    PlayerProfile& profile,
    const DevConsoleGameplaySnapshot* gameplaySnapshot) {
    if (commandLine == nullptr || commandLine[0] == '\0') {
        return;
    }
    char messageBuffer[DEV_CONSOLE_LOG_LINE_SIZE];
    messageBuffer[0] = '\0';
    if (devConsoleParseProfileSetCommand(commandLine, draft, profile, messageBuffer, sizeof(messageBuffer))) {
        devConsoleLogAppend(log, messageBuffer);
        return;
    }
    const char* cursor = commandLine;
    char token[64];
    if (!readToken(cursor, token, sizeof(token))) {
        return;
    }
    if (std::strcmp(token, "help") == 0) {
        logHelp(log);
        return;
    }
    if (std::strcmp(token, "log") == 0) {
        char subToken[64];
        if (readToken(cursor, subToken, sizeof(subToken)) && std::strcmp(subToken, "clear") == 0) {
            devConsoleLogClear(log);
            devConsoleLogAppend(log, "Log cleared.");
        }
        return;
    }
    if (std::strcmp(token, "profile") == 0) {
        char subToken[64];
        if (readToken(cursor, subToken, sizeof(subToken)) && std::strcmp(subToken, "dump") == 0) {
            logProfileDump(log, draft, profile);
        } else if (messageBuffer[0] != '\0') {
            devConsoleLogAppend(log, messageBuffer);
        }
        return;
    }
    if (std::strcmp(token, "draft") == 0) {
        char subToken[64];
        if (readToken(cursor, subToken, sizeof(subToken)) && std::strcmp(subToken, "show") == 0) {
            logDraftFields(log, draft);
        }
        return;
    }
    if (std::strcmp(token, "network") == 0) {
        char subToken[64];
        if (readToken(cursor, subToken, sizeof(subToken)) && std::strcmp(subToken, "show") == 0) {
            char lineOne[DEV_CONSOLE_LOG_LINE_SIZE];
            std::snprintf(
                lineOne,
                sizeof(lineOne),
                "ethnic=%.2f political=%.2f law=%.2f business=%.2f import=%.2f",
                profile.networkAccess.ethnicNetwork,
                profile.networkAccess.politicalMachine,
                profile.networkAccess.lawEnforcementChannel,
                profile.networkAccess.businessAssociation,
                profile.networkAccess.importPipeline);
            logProfileAxis(log, "[Network Access]", lineOne, "");
        }
        return;
    }
    if (std::strcmp(token, "legitimacy") == 0) {
        char subToken[64];
        if (readToken(cursor, subToken, sizeof(subToken)) && std::strcmp(subToken, "show") == 0) {
            char lineOne[DEV_CONSOLE_LOG_LINE_SIZE];
            std::snprintf(
                lineOne,
                sizeof(lineOne),
                "policeDecay=%.2f shellCo=%.2f publicJob=%.2f suspicion=%.2f",
                profile.legitimacy.policeAttentionDecay,
                profile.legitimacy.shellCompanyEase,
                profile.legitimacy.publicFacingJobAccess,
                profile.legitimacy.mainstreamSuspicion);
            logProfileAxis(log, "[Legitimacy]", lineOne, "");
        }
        return;
    }
    if (std::strcmp(token, "loyalty") == 0) {
        char subToken[64];
        if (readToken(cursor, subToken, sizeof(subToken)) && std::strcmp(subToken, "show") == 0) {
            char lineOne[DEV_CONSOLE_LOG_LINE_SIZE];
            std::snprintf(
                lineOne,
                sizeof(lineOne),
                "ethnicResist=%.2f kinPref=%.2f detectRisk=%.2f individual=%.2f",
                profile.loyaltyBias.ethnicFactionResistance,
                profile.loyaltyBias.kinAlliancePreference,
                profile.loyaltyBias.mainstreamDetectionRisk,
                profile.loyaltyBias.individualisticLoyalty);
            logProfileAxis(log, "[Loyalty Bias]", lineOne, "");
        }
        return;
    }
    if (std::strcmp(token, "culture") == 0) {
        char subToken[64];
        if (readToken(cursor, subToken, sizeof(subToken)) && std::strcmp(subToken, "show") == 0) {
            char lineOne[DEV_CONSOLE_LOG_LINE_SIZE];
            std::snprintf(
                lineOne,
                sizeof(lineOne),
                "inGroup=%.2f outGroup=%.2f penalty=%.2f language=%.2f translate=%.2f",
                profile.culturalCompetency.inGroupNegotiation,
                profile.culturalCompetency.outGroupNegotiation,
                profile.culturalCompetency.crossEthnicPenalty,
                profile.culturalCompetency.languageAccess,
                profile.culturalCompetency.translateBonus);
            logProfileAxis(log, "[Cultural Competency]", lineOne, "");
        }
        return;
    }
    if (std::strcmp(token, "paths") == 0) {
        char subToken[64];
        if (readToken(cursor, subToken, sizeof(subToken)) && std::strcmp(subToken, "show") == 0) {
            char lineOne[DEV_CONSOLE_LOG_LINE_SIZE];
            std::snprintf(
                lineOne,
                sizeof(lineOne),
                "street=%.2f organizer=%.2f institutional=%.2f corporate=%.2f",
                profile.opportunityPaths.streetCrimePath,
                profile.opportunityPaths.organizerPath,
                profile.opportunityPaths.institutionalPath,
                profile.opportunityPaths.corporatePath);
            logProfileAxis(log, "[Opportunity Paths]", lineOne, "");
        }
        return;
    }
    if (std::strcmp(token, "wallet") == 0) {
        char subToken[64];
        if (!readToken(cursor, subToken, sizeof(subToken)) || std::strcmp(subToken, "show") != 0) {
            return;
        }
        if (gameplaySnapshot == nullptr || !gameplaySnapshot->isWorldReady || gameplaySnapshot->playerWallet == nullptr) {
            devConsoleLogAppend(log, "wallet show requires an active in-game session.");
            return;
        }
        logWalletFields(log, *gameplaySnapshot->playerWallet);
        return;
    }
    if (std::strcmp(token, "cities") == 0) {
        char subToken[64];
        if (!readToken(cursor, subToken, sizeof(subToken)) || std::strcmp(subToken, "show") != 0) {
            return;
        }
        if (gameplaySnapshot == nullptr || !gameplaySnapshot->isWorldReady || gameplaySnapshot->cityControlStore == nullptr) {
            devConsoleLogAppend(log, "cities show requires an active in-game session.");
            return;
        }
        logCityControlFields(log, *gameplaySnapshot->cityControlStore);
        return;
    }
    if (std::strcmp(token, "operations") == 0) {
        char subToken[64];
        if (!readToken(cursor, subToken, sizeof(subToken)) || std::strcmp(subToken, "show") != 0) {
            return;
        }
        if (gameplaySnapshot == nullptr || !gameplaySnapshot->isWorldReady || gameplaySnapshot->playerOperationsStore == nullptr) {
            devConsoleLogAppend(log, "operations show requires an active in-game session.");
            return;
        }
        logOperationsFields(log, *gameplaySnapshot->playerOperationsStore);
        return;
    }
    if (std::strcmp(token, "crew") == 0) {
        char subToken[64];
        if (!readToken(cursor, subToken, sizeof(subToken)) || std::strcmp(subToken, "show") != 0) {
            return;
        }
        if (gameplaySnapshot == nullptr || !gameplaySnapshot->isWorldReady || gameplaySnapshot->playerOrganizationStore == nullptr) {
            devConsoleLogAppend(log, "crew show requires an active in-game session.");
            return;
        }
        const PlayerOrganizationStore& organization = *gameplaySnapshot->playerOrganizationStore;
        char buffer[DEV_CONSOLE_LOG_LINE_SIZE];
        std::snprintf(
            buffer,
            sizeof(buffer),
            "tier=%s members=%d crew=%s org=%s",
            playerPowerTierToString(organization.powerTier),
            organization.crewMemberCount,
            organization.crewName,
            organization.organizationName);
        devConsoleLogAppend(log, buffer);
        return;
    }
    if (std::strcmp(token, "calendar") == 0) {
        char subToken[64];
        if (!readToken(cursor, subToken, sizeof(subToken))) {
            return;
        }
        if (gameplaySnapshot == nullptr || !gameplaySnapshot->isWorldReady || gameplaySnapshot->gameplayStores == nullptr) {
            devConsoleLogAppend(log, "calendar commands require an active in-game session.");
            return;
        }
        GameCalendarStore& calendar = gameplaySnapshot->gameplayStores->calendarStore;
        if (std::strcmp(subToken, "show") == 0) {
            char buffer[DEV_CONSOLE_LOG_LINE_SIZE];
            formatCalendarDateLabel(calendar, buffer, sizeof(buffer));
            devConsoleLogAppend(log, buffer);
            std::snprintf(
                buffer,
                sizeof(buffer),
                "hour=%d weekHours=%d/%d worked=%d",
                calendar.hourOfDay,
                calendar.hoursWorkedThisWeek,
                calendar.scheduledHoursThisWeek,
                calendar.hoursWorkedThisWeek);
            devConsoleLogAppend(log, buffer);
            return;
        }
        if (std::strcmp(subToken, "hours") == 0) {
            char valueToken[64];
            if (!readToken(cursor, valueToken, sizeof(valueToken))) {
                devConsoleLogAppend(log, "Usage: calendar hours <0-60>");
                return;
            }
            calendar.scheduledHoursThisWeek = std::atoi(valueToken);
            devConsoleLogAppend(log, "Updated scheduled hours this week.");
            return;
        }
        if (std::strcmp(subToken, "skipday") == 0) {
            advanceGameCalendar(calendar, CALENDAR_TICKS_PER_DAY);
            devConsoleLogAppend(log, "Skipped one simulation day.");
            return;
        }
        return;
    }
    if (std::strcmp(token, "counsel") == 0) {
        char subToken[64];
        if (!readToken(cursor, subToken, sizeof(subToken)) || std::strcmp(subToken, "show") != 0) {
            return;
        }
        if (gameplaySnapshot == nullptr || !gameplaySnapshot->isWorldReady || gameplaySnapshot->gameplayStores == nullptr) {
            devConsoleLogAppend(log, "counsel show requires an active in-game session.");
            return;
        }
        const PlayerLegalCounselStore& counsel = gameplaySnapshot->gameplayStores->legalCounselStore;
        const LawyerTierDefinition* lawyer = getLawyerTierDefinition(counsel.hiredLawyerTier);
        char buffer[DEV_CONSOLE_LOG_LINE_SIZE];
        std::snprintf(buffer, sizeof(buffer), "counsel=%s retainer=%s", lawyer->displayName, counsel.hasRetainer ? "yes" : "no");
        devConsoleLogAppend(log, buffer);
        return;
    }
    if (std::strcmp(token, "intel") == 0) {
        char subToken[64];
        if (!readToken(cursor, subToken, sizeof(subToken))) {
            return;
        }
        if (gameplaySnapshot == nullptr || !gameplaySnapshot->isWorldReady
            || gameplaySnapshot->gameplayStores == nullptr
            || gameplaySnapshot->playerLawEnforcementStore == nullptr
            || gameplaySnapshot->playerWallet == nullptr
            || gameplaySnapshot->characterAgentStore == nullptr) {
            devConsoleLogAppend(log, "intel commands require an active in-game session.");
            return;
        }
        if (std::strcmp(subToken, "bribe") == 0) {
            GameCalendarStore calendarStore{};
            const CovertActionResult actionResult = executeCovertActionWithReason(
                CovertActionKind::BribePolice,
                ActionReasonId::BribeLookOtherWay,
                BEAT_COP_AGENT_SLOT_INDEX,
                false,
                gameplaySnapshot->gameplayStores->lawIntelStore,
                *gameplaySnapshot->playerLawEnforcementStore,
                *gameplaySnapshot->characterAgentStore,
                *gameplaySnapshot->playerWallet,
                *gameplaySnapshot->playerOrganizationStore,
                gameplaySnapshot->gameplayStores->narrativeArchiveStore,
                calendarStore,
                gameplaySnapshot->tickCount,
                gameplaySnapshot->tickCount);
            devConsoleLogAppend(log, actionResult.succeeded ? "Bribe succeeded — logged with reason." : "Bribe failed.");
            return;
        }
        if (std::strcmp(subToken, "flag") == 0) {
            char slotToken[64];
            char flagToken[64];
            if (!readToken(cursor, slotToken, sizeof(slotToken)) || !readToken(cursor, flagToken, sizeof(flagToken))) {
                devConsoleLogAppend(log, "Usage: intel flag <slot> <betrayed|snitch>");
                return;
            }
            const int32_t slotIndex = std::atoi(slotToken);
            if (slotIndex < 0 || slotIndex >= MAX_CHARACTER_AGENT_COUNT) {
                devConsoleLogAppend(log, "Invalid agent slot.");
                return;
            }
            CharacterAgentState& state = gameplaySnapshot->characterAgentStore->states[slotIndex];
            if (std::strcmp(flagToken, "betrayed") == 0) {
                setAgentRelationEvent(state, AgentRelationEventFlags::BetrayedPlayer);
            } else if (std::strcmp(flagToken, "snitch") == 0) {
                setAgentRelationEvent(state, AgentRelationEventFlags::SnitchedToPolice);
            } else {
                devConsoleLogAppend(log, "Unknown flag token.");
                return;
            }
            devConsoleLogAppend(log, "Relation flag applied.");
            return;
        }
        return;
    }
    if (std::strcmp(token, "law") == 0) {
        char subToken[64];
        if (!readToken(cursor, subToken, sizeof(subToken)) || std::strcmp(subToken, "show") != 0) {
            return;
        }
        if (gameplaySnapshot == nullptr || !gameplaySnapshot->isWorldReady || gameplaySnapshot->playerLawEnforcementStore == nullptr) {
            devConsoleLogAppend(log, "law show requires an active in-game session.");
            return;
        }
        const PlayerLawEnforcementStore& law = *gameplaySnapshot->playerLawEnforcementStore;
        char buffer[DEV_CONSOLE_LOG_LINE_SIZE];
        std::snprintf(
            buffer,
            sizeof(buffer),
            "heat=%d tier=%s evidence=%d warrants=%d witnesses=%d",
            law.personalHeat,
            getPoliceInvestigationLabel(law.investigationTier),
            law.evidenceScore,
            law.activeWarrantCount,
            law.witnessCount);
        devConsoleLogAppend(log, buffer);
        return;
    }
    if (std::strcmp(token, "justice") == 0) {
        char subToken[64];
        if (!readToken(cursor, subToken, sizeof(subToken))) {
            return;
        }
        if (gameplaySnapshot == nullptr || !gameplaySnapshot->isWorldReady
            || gameplaySnapshot->playerCriminalJusticeStore == nullptr
            || gameplaySnapshot->playerLawEnforcementStore == nullptr) {
            devConsoleLogAppend(log, "justice commands require an active in-game session.");
            return;
        }
        PlayerCriminalJusticeStore& justice = *gameplaySnapshot->playerCriminalJusticeStore;
        PlayerLawEnforcementStore& law = *gameplaySnapshot->playerLawEnforcementStore;
        if (std::strcmp(subToken, "show") == 0) {
            char buffer[DEV_CONSOLE_LOG_LINE_SIZE];
            std::snprintf(
                buffer,
                sizeof(buffer),
                "phase=%s bond=%lld arrests=%d probation=%d parole=%d",
                custodyPhaseToString(getPlayerCustodyPhase(justice)),
                static_cast<long long>(justice.bondCents),
                justice.arrestCount,
                justice.probationTicksRemaining,
                justice.paroleTicksRemaining);
            devConsoleLogAppend(log, buffer);
            return;
        }
        if (std::strcmp(subToken, "release") == 0) {
            releasePlayerFromCustody(justice, law);
            devConsoleLogAppend(log, "Player released from custody.");
            return;
        }
        if (std::strcmp(subToken, "arrest") == 0) {
            char tierToken[32];
            CrimeLegalTier tier = CrimeLegalTier::Street;
            if (readToken(cursor, tierToken, sizeof(tierToken))) {
                if (std::strcmp(tierToken, "petty") == 0) {
                    tier = CrimeLegalTier::PettyStreet;
                } else if (std::strcmp(tierToken, "org") == 0) {
                    tier = CrimeLegalTier::Organization;
                } else if (std::strcmp(tierToken, "financial") == 0) {
                    tier = CrimeLegalTier::Financial;
                }
            }
            beginPlayerArrest(justice, law, tier, gameplaySnapshot->tickCount, "Dev console arrest");
            devConsoleLogAppend(log, "Player arrested.");
            return;
        }
        return;
    }
    if (std::strcmp(token, "event") == 0) {
        char subToken[64];
        if (!readToken(cursor, subToken, sizeof(subToken))) {
            return;
        }
        if (gameplaySnapshot == nullptr || !gameplaySnapshot->isWorldReady || gameplaySnapshot->worldEventStore == nullptr
            || gameplaySnapshot->playerOperationsStore == nullptr || gameplaySnapshot->playerWallet == nullptr
            || gameplaySnapshot->characterAgentStore == nullptr) {
            devConsoleLogAppend(log, "event commands require an active in-game session.");
            return;
        }
        if (std::strcmp(subToken, "flags") == 0) {
            char buffer[DEV_CONSOLE_LOG_LINE_SIZE];
            std::snprintf(buffer, sizeof(buffer), "worldFlags=0x%08X message=%s", gameplaySnapshot->worldEventStore->worldFlags, gameplaySnapshot->worldEventStore->lastPlayerMessage);
            devConsoleLogAppend(log, buffer);
            return;
        }
        if (std::strcmp(subToken, "fire") == 0) {
            char eventId[64];
            if (!readToken(cursor, eventId, sizeof(eventId))) {
                devConsoleLogAppend(log, "Usage: event fire <event_id>");
                return;
            }
            const int32_t definitionIndex = findWorldEventDefinitionIndexById(eventId);
            if (definitionIndex < 0) {
                devConsoleLogAppend(log, "Unknown event id.");
                return;
            }
            const WorldEventDefinition* definition = getWorldEventDefinition(definitionIndex);
            if (definition == nullptr) {
                return;
            }
            for (int32_t effectIndex = 0; effectIndex < 4; ++effectIndex) {
                if (definition->effects[effectIndex].kind == WorldEventEffectKind::None) {
                    continue;
                }
                applyWorldEventEffect(
                    definition->effects[effectIndex],
                    *gameplaySnapshot->worldEventStore,
                    *gameplaySnapshot->playerOperationsStore,
                    *gameplaySnapshot->playerWallet,
                    *gameplaySnapshot->characterAgentStore,
                    gameplaySnapshot->tickCount);
            }
            markWorldEventFired(*gameplaySnapshot->worldEventStore, definitionIndex, gameplaySnapshot->tickCount);
            devConsoleLogAppend(log, "Event fired.");
            return;
        }
        return;
    }
    if (std::strcmp(token, "agent") == 0) {
        char subToken[64];
        if (!readToken(cursor, subToken, sizeof(subToken)) || std::strcmp(subToken, "deactivate") != 0) {
            return;
        }
        char slotToken[16];
        if (!readToken(cursor, slotToken, sizeof(slotToken))) {
            devConsoleLogAppend(log, "Usage: agent deactivate <slot>");
            return;
        }
        if (gameplaySnapshot == nullptr || !gameplaySnapshot->isWorldReady || gameplaySnapshot->characterAgentStore == nullptr) {
            devConsoleLogAppend(log, "agent deactivate requires an active in-game session.");
            return;
        }
        const int32_t slotIndex = std::atoi(slotToken);
        if (slotIndex < 0 || slotIndex >= MAX_CHARACTER_AGENT_COUNT) {
            devConsoleLogAppend(log, "Invalid agent slot.");
            return;
        }
        gameplaySnapshot->characterAgentStore->states[slotIndex].isActive = false;
        devConsoleLogAppend(log, "Agent slot deactivated.");
        return;
    }
    if (std::strcmp(token, "agents") == 0) {
        char subToken[64];
        if (!readToken(cursor, subToken, sizeof(subToken)) || std::strcmp(subToken, "show") != 0) {
            return;
        }
        if (gameplaySnapshot == nullptr || !gameplaySnapshot->isWorldReady || gameplaySnapshot->characterAgentStore == nullptr) {
            devConsoleLogAppend(log, "agents show requires an active in-game session.");
            return;
        }
        logAgentsFields(log, *gameplaySnapshot->characterAgentStore);
        return;
    }
    devConsoleLogAppend(log, "Unknown command. Type help.");
}

namespace {

const char* mobilityAssetLabel(MobilityAsset asset) {
    switch (asset) {
        case MobilityAsset::Bicycle: return "Bicycle";
        case MobilityAsset::Vehicle: return "Vehicle";
        default: return "On foot";
    }
}

bool inspectorFilterMatches(const char* name, const char* role, const char* filter) {
    if (filter == nullptr || filter[0] == '\0') {
        return true;
    }
    char filterLower[64];
    int32_t filterLen = 0;
    for (; filter[filterLen] != '\0' && filterLen < 63; ++filterLen) {
        filterLower[filterLen] = static_cast<char>(std::tolower(static_cast<unsigned char>(filter[filterLen])));
    }
    filterLower[filterLen] = '\0';
    char haystack[96];
    std::snprintf(haystack, sizeof(haystack), "%s %s", name != nullptr ? name : "", role != nullptr ? role : "");
    for (int32_t i = 0; haystack[i] != '\0'; ++i) {
        haystack[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(haystack[i])));
    }
    return std::strstr(haystack, filterLower) != nullptr;
}

void renderAgentDetail(const DevConsoleGameplaySnapshot& snapshot, int32_t agentIndex) {
    if (snapshot.characterAgentStore == nullptr) {
        ImGui::TextUnformatted("No world loaded.");
        return;
    }
    const CharacterAgentStore& store = *snapshot.characterAgentStore;
    if (agentIndex < 0 || agentIndex >= MAX_CHARACTER_AGENT_COUNT || !store.states[agentIndex].isActive) {
        ImGui::TextUnformatted("Select a character from the list to inspect their full profile.");
        return;
    }
    const CharacterAgentState& agent = store.states[agentIndex];
    const char* name = nullptr;
    const char* role = nullptr;
    tryGetAgentDisplayLabels(store, agentIndex, name, role);
    ImGui::Text("%s", name != nullptr ? name : "Unknown");
    ImGui::TextDisabled("Slot %d  |  %s", agentIndex, role != nullptr ? role : "");
    ImGui::Separator();
    ImGui::Text("Archetype:  %s", agentArchetypeToLabel(agent.generatedArchetype));
    ImGui::Text("Motive:     %s", agentMotiveToLabel(agent.generatedMotive));
    ImGui::Text("Objective:  %s", agentObjectiveToLabel(agent.currentObjective));
    ImGui::Text("Trait:      %s", agentTraitToLabel(agent.generatedTrait));
    ImGui::Text("Emotion:    %s", agentEmotionToLabel(agent.currentEmotion));
    ImGui::Text("Activity:   %s%s", getActivityDisplayLabel(agent.currentActivity), agent.isVisibleOnMap ? "  (on map)" : "");
    ImGui::Separator();
    ImGui::Text("Position:   (%d, %d)", agent.currentTileX, agent.currentTileY);
    ImGui::Text("Home:       region %d, property %d", static_cast<int32_t>(agent.homeRegionId), agent.homePropertyIndex);
    if (agent.workplaceBusinessIndex >= 0) {
        const BusinessNodeDefinition* business = getBusinessNodeDefinition(agent.workplaceBusinessIndex);
        ImGui::Text("Workplace:  %s", business != nullptr ? business->fullName : "(unknown)");
    } else {
        ImGui::TextUnformatted("Workplace:  none");
    }
    ImGui::Text("Cash:       $%.2f", static_cast<double>(agent.cashCents) / 100.0);
    ImGui::Text("Mobility:   %s", mobilityAssetLabel(agent.mobilityAsset));
    if (agent.currentActivity == AgentActivity::Incarcerated) {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.4f, 1.0f), "Status:     In custody");
    }
    if (agent.wantedLevel > 0) {
        ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), "Wanted level: %d", static_cast<int32_t>(agent.wantedLevel));
    }
    ImGui::Separator();
    ImGui::TextUnformatted("Disposition toward player:");
    ImGui::Text("  opinion %d   trust %d   fear %d   respect %d", agent.opinionOfPlayer, agent.trust, agent.fear, agent.respect);
    ImGui::Separator();
    if (snapshot.agentRelationshipGraph != nullptr) {
        const int32_t tieCount = getAgentTieCount(*snapshot.agentRelationshipGraph, agentIndex);
        ImGui::Text("Relationships (%d):", tieCount);
        for (int32_t tieIndex = 0; tieIndex < tieCount; ++tieIndex) {
            const AgentTie* tie = getAgentTie(*snapshot.agentRelationshipGraph, agentIndex, tieIndex);
            if (tie == nullptr || tie->targetAgentIndex < 0) {
                continue;
            }
            const char* tieName = nullptr;
            const char* tieRole = nullptr;
            tryGetAgentDisplayLabels(store, tie->targetAgentIndex, tieName, tieRole);
            ImGui::BulletText(
                "%s - %s (affinity %d)",
                tieName != nullptr ? tieName : "(unknown)",
                agentTieKindToLabel(static_cast<AgentTieKind>(tie->kind)),
                static_cast<int32_t>(tie->affinity));
        }
    }
}

void renderCharacterInspector(DevConsoleState& state, const DevConsoleGameplaySnapshot* snapshot) {
    if (!state.isInspectorVisible) {
        return;
    }
    if (snapshot == nullptr || !snapshot->isWorldReady || snapshot->characterAgentStore == nullptr) {
        return;
    }
    ImGui::SetNextWindowSize(ImVec2(620.0f, 440.0f), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Character Inspector", &state.isInspectorVisible)) {
        ImGui::End();
        return;
    }
    const CharacterAgentStore& store = *snapshot->characterAgentStore;
    int32_t activeCount = 0;
    for (int32_t i = 0; i < MAX_CHARACTER_AGENT_COUNT; ++i) {
        if (store.states[i].isActive) {
            activeCount += 1;
        }
    }
    ImGui::Text("Active characters: %d", activeCount);
    ImGui::InputTextWithHint("##inspectorFilter", "filter by name or role", state.inspectorFilter, sizeof(state.inspectorFilter));
    ImGui::Separator();
    ImGui::BeginChild("InspectorList", ImVec2(240.0f, 0.0f), true);
    for (int32_t i = 0; i < MAX_CHARACTER_AGENT_COUNT; ++i) {
        if (!store.states[i].isActive) {
            continue;
        }
        const char* name = nullptr;
        const char* role = nullptr;
        if (!tryGetAgentDisplayLabels(store, i, name, role)) {
            continue;
        }
        if (!inspectorFilterMatches(name, role, state.inspectorFilter)) {
            continue;
        }
        char label[128];
        std::snprintf(label, sizeof(label), "%s  [%s]##agent%d", name != nullptr ? name : "?", role != nullptr ? role : "", i);
        if (ImGui::Selectable(label, state.selectedAgentIndex == i)) {
            state.selectedAgentIndex = i;
        }
    }
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild("InspectorDetail", ImVec2(0.0f, 0.0f), true);
    renderAgentDetail(*snapshot, state.selectedAgentIndex);
    ImGui::EndChild();
    ImGui::End();
}

} // namespace

void devConsoleRender(
    DevConsoleState& state,
    DevConsoleLog& log,
    CharacterDraft& draft,
    PlayerProfile& profile,
    const DevConsoleGameplaySnapshot* gameplaySnapshot) {
    if (!state.isVisible) {
        return;
    }
    ImGui::SetNextWindowSize(ImVec2(640.0f, 360.0f), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin(DEV_CONSOLE_WINDOW_NAME, &state.isVisible)) {
        ImGui::End();
        renderCharacterInspector(state, gameplaySnapshot);
        return;
    }
    ImGui::Checkbox("Character Inspector", &state.isInspectorVisible);
    ImGui::Separator();
    const float footerHeight = ImGui::GetFrameHeightWithSpacing();
    ImGui::BeginChild("DevConsoleLog", ImVec2(0.0f, -footerHeight), true, ImGuiWindowFlags_HorizontalScrollbar);
    const int32_t startLine = log.lineCount < DEV_CONSOLE_LOG_LINE_COUNT ? 0 : log.writeIndex;
    for (int32_t offset = 0; offset < log.lineCount; ++offset) {
        const int32_t lineIndex = (startLine + offset) % DEV_CONSOLE_LOG_LINE_COUNT;
        ImGui::TextUnformatted(log.lines[lineIndex]);
    }
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }
    ImGui::EndChild();
    ImGui::Separator();
    ImGuiInputTextFlags inputFlags = ImGuiInputTextFlags_EnterReturnsTrue;
    if (state.requestFocusInput) {
        ImGui::SetKeyboardFocusHere();
        state.requestFocusInput = false;
    }
    if (ImGui::InputText("##DevConsoleInput", state.inputBuffer, sizeof(state.inputBuffer), inputFlags)) {
        devConsoleLogAppend(log, state.inputBuffer);
        devConsoleExecuteCommand(log, state.inputBuffer, draft, profile, gameplaySnapshot);
        state.inputBuffer[0] = '\0';
    }
    ImGui::End();
    renderCharacterInspector(state, gameplaySnapshot);
}

} // namespace Core
