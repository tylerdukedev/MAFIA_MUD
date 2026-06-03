#include "character/character_social_network.h"
#include "character/character_types.h"
#include <catch2/catch_test_macros.hpp>

using namespace Core;

TEST_CASE("Social network roll can leave player without family or friends", "[character_social_network]") {
    bool sawNoContacts = false;
    bool sawFamilyOnly = false;
    bool sawBoth = false;
    for (uint64_t seed = 1; seed < 400; ++seed) {
        CharacterDraft draft{};
        draft.generationId = GenerationId::Immigrant;
        draft.backgroundId = BackgroundId::Bookkeeper;
        draft.characterRollSeed = seed;
        rollCharacterSocialNetwork(draft);
        if (!draft.hasFamilyInCountry && !draft.hasFriendsInCountry) {
            sawNoContacts = true;
        }
        if (draft.hasFamilyInCountry && !draft.hasFriendsInCountry) {
            sawFamilyOnly = true;
        }
        if (draft.hasFamilyInCountry && draft.hasFriendsInCountry) {
            sawBoth = true;
        }
    }
    REQUIRE(sawNoContacts);
    REQUIRE(sawFamilyOnly);
    REQUIRE(sawBoth);
}

TEST_CASE("Personal contacts spawn as generated characters", "[character_social_network]") {
    CharacterDraft draft{};
    draft.heritageId = HeritageId::Italian;
    draft.characterRollSeed = 99123ULL;
    draft.hasFamilyInCountry = true;
    draft.hasFriendsInCountry = true;
    CharacterAgentStore store{};
    initializeCharacterAgentStore(store);
    spawnPersonalContactsFromDraft(draft, store);
    REQUIRE(store.states[FAMILY_AGENT_SLOT_INDEX].isActive);
    REQUIRE(store.states[FAMILY_AGENT_SLOT_INDEX].hasGeneratedIdentity);
    REQUIRE(store.states[FRIEND_AGENT_SLOT_INDEX].isActive);
    REQUIRE(store.states[FRIEND_AGENT_SLOT_INDEX].hasGeneratedIdentity);
    REQUIRE(store.states[FIRST_COMMUNITY_AGENT_SLOT_INDEX].isActive);
}
