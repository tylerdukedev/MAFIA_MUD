#include "character/profile_builder.h"
#include <catch2/catch_test_macros.hpp>
#include <cstring>

using namespace Core;

TEST_CASE("ProfileBuilder generation tradeoffs favor different capabilities", "[character][profile]") {
    CharacterDraft immigrantDraft{};
    immigrantDraft.generationId = GenerationId::Immigrant;
    immigrantDraft.nationalityId = NationalityId::American;
    immigrantDraft.heritageId = HeritageId::Italian;
    immigrantDraft.age = 21;
    immigrantDraft.backgroundId = BackgroundId::StreetHustler;
    CharacterDraft thirdGenDraft = immigrantDraft;
    thirdGenDraft.generationId = GenerationId::ThirdGenerationPlus;
    const PlayerProfile immigrantProfile = buildPlayerProfile(immigrantDraft);
    const PlayerProfile thirdGenProfile = buildPlayerProfile(thirdGenDraft);
    REQUIRE(immigrantProfile.networkAccess.ethnicNetwork > thirdGenProfile.networkAccess.ethnicNetwork);
    REQUIRE(thirdGenProfile.legitimacy.publicFacingJobAccess > immigrantProfile.legitimacy.publicFacingJobAccess);
    REQUIRE(immigrantProfile.opportunityPaths.streetCrimePath > immigrantProfile.opportunityPaths.corporatePath);
    REQUIRE(thirdGenProfile.opportunityPaths.corporatePath > thirdGenProfile.opportunityPaths.streetCrimePath);
}

TEST_CASE("ProfileBuilder description builder produces text", "[character][profile]") {
    CharacterDraft draft{};
    initializeCharacterDraftDefaults(draft);
    std::snprintf(draft.nameBuffer, sizeof(draft.nameBuffer), "%s", "Vito Russo");
    draft.generationId = GenerationId::FirstGeneration;
    draft.heritageId = HeritageId::Italian;
    draft.nationalityId = NationalityId::American;
    draft.age = 22;
    draft.selectedBoroughIndex = 1;
    char descriptionBuffer[256];
    buildCharacterDescription(descriptionBuffer, sizeof(descriptionBuffer), draft);
    REQUIRE(std::strlen(descriptionBuffer) > 0);
    REQUIRE(std::strstr(descriptionBuffer, "Vito Russo") != nullptr);
    REQUIRE(std::strstr(descriptionBuffer, "22") != nullptr);
}
