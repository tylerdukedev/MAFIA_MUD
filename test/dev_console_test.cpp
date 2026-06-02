#include "dev/dev_console.h"
#include "character/profile_builder.h"
#include <catch2/catch_test_macros.hpp>
#include <cstring>

using namespace Core;

TEST_CASE("DevConsole profile set generation rebuilds profile", "[dev][console]") {
    CharacterDraft draft{};
    initializeCharacterDraftDefaults(draft);
    PlayerProfile profile = buildPlayerProfile(draft);
    char messageBuffer[128];
    const bool didSet = devConsoleParseProfileSetCommand(
        "profile set generation immigrant",
        draft,
        profile,
        messageBuffer,
        sizeof(messageBuffer));
    REQUIRE(didSet);
    REQUIRE(draft.generationId == GenerationId::Immigrant);
    REQUIRE(profile.draft.generationId == GenerationId::Immigrant);
    REQUIRE(profile.networkAccess.ethnicNetwork > profile.opportunityPaths.corporatePath);
}

TEST_CASE("DevConsole profile set rejects invalid age", "[dev][console]") {
    CharacterDraft draft{};
    initializeCharacterDraftDefaults(draft);
    PlayerProfile profile = buildPlayerProfile(draft);
    char messageBuffer[128];
    const bool didSet = devConsoleParseProfileSetCommand(
        "profile set age 40",
        draft,
        profile,
        messageBuffer,
        sizeof(messageBuffer));
    REQUIRE_FALSE(didSet);
    REQUIRE(std::strstr(messageBuffer, "Age must be") != nullptr);
}
