#include "dev/dev_console.h"
#include "character/character_tables.h"
#include "character/profile_builder.h"
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

void logHelp(DevConsoleLog& log) {
    devConsoleLogAppend(log, "Commands:");
    devConsoleLogAppend(log, "  help");
    devConsoleLogAppend(log, "  log clear");
    devConsoleLogAppend(log, "  profile dump | draft show");
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
    PlayerProfile& profile) {
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
    devConsoleLogAppend(log, "Unknown command. Type help.");
}

void devConsoleRender(DevConsoleState& state, DevConsoleLog& log, CharacterDraft& draft, PlayerProfile& profile) {
    if (!state.isVisible) {
        return;
    }
    ImGui::SetNextWindowSize(ImVec2(640.0f, 360.0f), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin(DEV_CONSOLE_WINDOW_NAME, &state.isVisible)) {
        ImGui::End();
        return;
    }
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
        devConsoleExecuteCommand(log, state.inputBuffer, draft, profile);
        state.inputBuffer[0] = '\0';
    }
    ImGui::End();
}

} // namespace Core
