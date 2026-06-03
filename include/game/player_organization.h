#pragma once

#include "character/player_profile.h"
#include "game/player_operations.h"
#include "game/player_wallet.h"
#include "sim/character_agent.h"
#include <cstdint>

namespace Core {

constexpr int32_t MAX_CREW_MEMBER_COUNT = 3;
constexpr int32_t MIN_CREW_MEMBERS_TO_FORM = 2;
constexpr int32_t CREW_RECRUIT_MIN_OPINION = 12;
constexpr int32_t CREW_RECRUIT_MIN_TRUST = 38;
constexpr int32_t CREW_RECRUIT_MIN_RESPECT = 32;
constexpr int32_t CREW_RECRUIT_MIN_LOYALTY_SCORE = 35;
constexpr int32_t ORG_MIN_REPUTATION_SCORE = 14;
constexpr int32_t ORG_MIN_LIFETIME_CRIME_CENTS = 1500;
constexpr float ORG_MIN_NETWORK_ACCESS = 0.18f;
constexpr float ORG_MIN_STREET_CRIME_PATH = 0.32f;

enum class PlayerPowerTier : uint8_t {
    Solo = 0,
    Crew = 1,
    Organization = 2,
};

enum class CrewRecruitLockReason : uint8_t {
    None = 0,
    NotSoloOrCrewBuilding = 1,
    AlreadyInCrew = 2,
    AgentInactive = 3,
    NotRecruitableRole = 4,
    LowOpinion = 5,
    LowTrust = 6,
    LowRespect = 7,
    LowLoyalty = 8,
    CrewFull = 9,
};

enum class StreetCrimeTier : uint8_t;

enum class OrganizationFormLockReason : uint8_t {
    None = 0,
    NotCrewTier = 1,
    InsufficientMembers = 2,
    InsufficientNetwork = 3,
    InsufficientStreetPath = 4,
    InsufficientReputation = 5,
    InsufficientCrimeRecord = 6,
    HeatTooHigh = 7,
    ActiveWarrant = 8,
};

struct PlayerOrganizationStore {
    PlayerPowerTier powerTier = PlayerPowerTier::Solo;
    int32_t crewMemberAgentIndices[MAX_CREW_MEMBER_COUNT]{-1, -1, -1};
    int32_t crewMemberCount = 0;
    char crewName[32]{};
    char organizationName[48]{};
    char organizationFront[48]{};
    uint64_t crewFormedTick = 0;
    uint64_t organizationFormedTick = 0;
};

void resetPlayerOrganizationStore(PlayerOrganizationStore& store);
const char* playerPowerTierToString(PlayerPowerTier tier);
int32_t computeAgentLoyaltyScore(const CharacterAgentState& state);
bool isAgentRecruitableForCrew(int32_t agentIndex);
bool isAgentInCrew(const PlayerOrganizationStore& store, int32_t agentIndex);
CrewRecruitLockReason evaluateCrewRecruitLock(
    const PlayerOrganizationStore& store,
    const CharacterAgentStore& agentStore,
    int32_t agentIndex);
int32_t rollCrewRecruitAcceptChance(
    const CharacterAgentState& state,
    const PlayerProfile& profile,
    int32_t pitchChoice,
    uint64_t worldSeed,
    int32_t agentIndex);
bool tryAddCrewMember(PlayerOrganizationStore& store, int32_t agentIndex);
bool tryFormalizeCrew(PlayerOrganizationStore& store, const char* crewNameInput, uint64_t tickCount);
struct PlayerLawEnforcementStore;

OrganizationFormLockReason evaluateOrganizationFormLock(
    const PlayerOrganizationStore& store,
    const PlayerLawEnforcementStore& lawStore,
    const PlayerProfile& profile,
    const PlayerWallet& wallet);
bool tryFormalizeOrganization(
    PlayerOrganizationStore& store,
    const char* organizationNameInput,
    const char* organizationFrontInput,
    uint64_t tickCount);
bool meetsStreetCrimeTierRequirement(PlayerPowerTier playerTier, StreetCrimeTier crimeTier);

} // namespace Core
