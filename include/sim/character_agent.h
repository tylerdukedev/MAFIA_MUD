#pragma once

#include <cstdint>

namespace Core {

constexpr int32_t MAX_CHARACTER_AGENT_COUNT = 32;
constexpr int32_t AGENT_OPINION_MIN = -100;
constexpr int32_t AGENT_OPINION_MAX = 100;

enum class AgentMotive : uint8_t {
    Survival = 0,
    Wealth = 1,
    Status = 2,
    Loyalty = 3,
    Revenge = 4,
};

enum class AgentEmotion : uint8_t {
    Calm = 0,
    Anxious = 1,
    Angry = 2,
    Grateful = 3,
    Suspicious = 4,
};

enum class AgentPersonalityTrait : uint8_t {
    Pragmatic = 0,
    Proud = 1,
    Paranoid = 2,
    Charitable = 3,
    Ruthless = 4,
};

struct AgentDefinition {
    const char* id;
    const char* displayName;
    const char* roleLabel;
    AgentMotive primaryMotive;
    AgentEmotion baselineEmotion;
    AgentPersonalityTrait primaryTrait;
    int32_t baselineOpinionOfPlayer;
};

struct CharacterAgentState {
    int32_t opinionOfPlayer = 0;
    int32_t trust = 50;
    int32_t fear = 10;
    int32_t respect = 40;
    AgentEmotion currentEmotion = AgentEmotion::Calm;
    bool isActive = false;
};

struct CharacterAgentStore {
    CharacterAgentState states[MAX_CHARACTER_AGENT_COUNT]{};
};

int32_t getCharacterAgentDefinitionCount();
const AgentDefinition* getCharacterAgentDefinition(int32_t agentIndex);
void initializeCharacterAgentStore(CharacterAgentStore& store);
void resetCharacterAgentStore(CharacterAgentStore& store);
const CharacterAgentState* getCharacterAgentState(const CharacterAgentStore& store, int32_t agentIndex);
void adjustAgentOpinion(CharacterAgentStore& store, int32_t agentIndex, int32_t delta);

} // namespace Core
