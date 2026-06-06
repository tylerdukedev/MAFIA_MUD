#pragma once

#include "game/shared_travel_state.h"
#include "game/travel_modes.h"
#include <cstdint>

namespace Core {

constexpr int32_t MAX_CHARACTER_AGENT_COUNT = 96;
constexpr int32_t AGENT_OPINION_MIN = -100;
constexpr int32_t AGENT_OPINION_MAX = 100;

enum class AgentMotive : uint8_t {
    Survival = 0,
    Wealth = 1,
    Status = 2,
    Loyalty = 3,
    Revenge = 4,
};

// Occupation / social role of an agent. Drives starting cash, motive bias,
// criminality, and behavior. Keep additions appended for save stability.
enum class AgentArchetype : uint8_t {
    Civilian = 0,
    Laborer = 1,
    Dockworker = 2,
    Shopkeeper = 3,
    Clerk = 4,
    Bartender = 5,
    Cabbie = 6,
    Landlord = 7,
    UnionMan = 8,
    Priest = 9,
    Doctor = 10,
    Patrolman = 11,
    Detective = 12,
    Politician = 13,
    Socialite = 14,
    NumbersRunner = 15,
    Bootlegger = 16,
    Enforcer = 17,
    Fence = 18,
    Loanshark = 19,
    Racketeer = 20,
    Vagrant = 21,
    Count = 22,
};

// Current short-horizon goal the agent is pursuing. Selected each decision
// window by a bounded motive/need scorer in npc_decision.
enum class AgentObjective : uint8_t {
    Settle = 0,
    EarnWage = 1,
    RestAtHome = 2,
    Socialize = 3,
    SeekIncome = 4,
    RunRacket = 5,
    LayLow = 6,
    Patrol = 7,
    SeekCare = 8,
    Count = 9,
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

enum class AgentActivity : uint8_t {
    Idle = 0,
    AtHome = 1,
    AtWork = 2,
    Traveling = 3,
    InBuilding = 4,
    Abroad = 5,
    Incarcerated = 6,
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
    bool hasGeneratedIdentity = false;
    char generatedDisplayName[32]{};
    char generatedRoleLabel[24]{};
    AgentMotive generatedMotive = AgentMotive::Loyalty;
    AgentPersonalityTrait generatedTrait = AgentPersonalityTrait::Pragmatic;
    AgentArchetype generatedArchetype = AgentArchetype::Civilian;
    AgentObjective currentObjective = AgentObjective::Settle;
    uint16_t relationEventFlags = 0;
    int32_t currentTileX = -1;
    int32_t currentTileY = -1;
    int32_t destinationTileX = -1;
    int32_t destinationTileY = -1;
    MobilityAsset mobilityAsset = MobilityAsset::None;
    int32_t travelPathTileCount = 0;
    int32_t travelPathTileX[NPC_MAX_TRAVEL_PATH_TILES]{};
    int32_t travelPathTileY[NPC_MAX_TRAVEL_PATH_TILES]{};
    uint64_t travelStartTick = 0;
    uint64_t travelCompleteTick = 0;
    TravelMode travelMode = TravelMode::Walk;
    int32_t travelDestTileX = -1;
    int32_t travelDestTileY = -1;
    AgentActivity currentActivity = AgentActivity::Idle;
    int32_t homePropertyIndex = -1;
    int32_t workplaceBusinessIndex = -1;
    int32_t cashCents = 0;
    uint64_t activityStartTick = 0;
    uint64_t lastWageTick = 0;
    uint64_t lastCrimeTick = 0;
    uint8_t homeRegionId = 0;
    int8_t wantedLevel = 0;
    bool isVisibleOnMap = false;
};

struct CharacterAgentStore {
    CharacterAgentState states[MAX_CHARACTER_AGENT_COUNT]{};
};

int32_t getCharacterAgentDefinitionCount();
const AgentDefinition* getCharacterAgentDefinition(int32_t agentIndex);
void initializeCharacterAgentStore(CharacterAgentStore& store);
void resetCharacterAgentStore(CharacterAgentStore& store);
const CharacterAgentState* getCharacterAgentState(const CharacterAgentStore& store, int32_t agentIndex);
void deriveRelationshipStatsFromOpinion(CharacterAgentState& state);
void adjustAgentOpinion(CharacterAgentStore& store, int32_t agentIndex, int32_t delta);
bool tryGetAgentDisplayLabels(
    const CharacterAgentStore& store,
    int32_t agentIndex,
    const char*& outDisplayName,
    const char*& outRoleLabel);
void setAgentPosition(CharacterAgentState& state, int32_t tileX, int32_t tileY);
void setAgentActivity(CharacterAgentState& state, AgentActivity activity, uint64_t tickCount);
bool isAgentVisibleOnMap(const CharacterAgentState& state);
void updateAgentMapVisibility(CharacterAgentState& state);
const char* agentArchetypeToLabel(AgentArchetype archetype);
const char* agentMotiveToLabel(AgentMotive motive);
const char* agentObjectiveToLabel(AgentObjective objective);
const char* agentTraitToLabel(AgentPersonalityTrait trait);
const char* agentEmotionToLabel(AgentEmotion emotion);
bool isCriminalArchetype(AgentArchetype archetype);
bool isLawEnforcementArchetype(AgentArchetype archetype);

} // namespace Core
