#include <catch2/catch_test_macros.hpp>
#include "character/character_name.h"

namespace Core {

TEST_CASE("sanitizeCharacterNamePart collapses whitespace", "[character_name]") {
    CharacterDraft draft{};
    std::snprintf(draft.firstName, sizeof(draft.firstName), "%s", "     Mario     ");
    std::snprintf(draft.lastName, sizeof(draft.lastName), "%s", "  Marizoni  ");
    normalizeCharacterDraftNames(draft);
    REQUIRE(std::string(draft.firstName) == "Mario");
    REQUIRE(std::string(draft.lastName) == "Marizoni");
    REQUIRE(std::string(draft.nameBuffer) == "Mario Marizoni");
}

TEST_CASE("isCharacterNameValid requires first and last", "[character_name]") {
    CharacterDraft draft{};
    REQUIRE_FALSE(isCharacterNameValid(draft));
    std::snprintf(draft.firstName, sizeof(draft.firstName), "%s", "Ana");
    REQUIRE_FALSE(isCharacterNameValid(draft));
    std::snprintf(draft.lastName, sizeof(draft.lastName), "%s", "Lopez");
    normalizeCharacterDraftNames(draft);
    REQUIRE(isCharacterNameValid(draft));
}

} // namespace Core
