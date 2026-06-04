#pragma once

#include "character/character_draft.h"
#include "sim/character_agent.h"
#include <cstdint>

namespace Core {

constexpr int32_t FAMILY_AGENT_SLOT_INDEX = 0;
constexpr int32_t FRIEND_AGENT_SLOT_INDEX = 1;
constexpr int32_t FIRST_COMMUNITY_AGENT_SLOT_INDEX = 2;
constexpr int32_t BEAT_COP_AGENT_SLOT_INDEX = FIRST_COMMUNITY_AGENT_SLOT_INDEX + 1;
constexpr int32_t RIVAL_AGENT_SLOT_INDEX = FIRST_COMMUNITY_AGENT_SLOT_INDEX + 3;
constexpr int32_t BOSS_AGENT_SLOT_INDEX = 6;

void rollCharacterSocialNetwork(CharacterDraft& draft);
void buildStartingContactPreviewStore(const CharacterDraft& draft, CharacterAgentStore& store);
void spawnPersonalContactsFromDraft(const CharacterDraft& draft, CharacterAgentStore& store);
void formatCharacterSocialSummary(const CharacterDraft& draft, char* outBuffer, size_t bufferSize);
bool hasPersonalLodgingOption(const CharacterDraft& draft);

} // namespace Core
