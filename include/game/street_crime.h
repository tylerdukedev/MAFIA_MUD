#pragma once

#include "character/character_social_network.h"
#include "character/player_profile.h"
#include "game/crime_legal_tier.h"
#include "game/player_criminal_justice.h"
#include "game/player_law_enforcement.h"
#include "game/player_organization.h"
#include "game/player_operations.h"
#include "game/player_wallet.h"
#include "sim/character_agent.h"
#include <cstdint>

namespace Core {

constexpr int32_t MAX_STREET_CRIME_COUNT = 8;
constexpr int32_t STREET_CRIME_BROKE_CASH_THRESHOLD_CENTS = 500;
enum class StreetCrimeTier : uint8_t {
    Solo = 0,
    Crew = 1,
    Organization = 2,
};

enum class StreetCrimeLockReason : uint8_t {
    None = 0,
    NeedsHeadquarters = 1,
    OnCooldown = 2,
    InsufficientStreetPath = 3,
    InsufficientNetwork = 4,
    InsufficientCriminalTrust = 5,
    InsufficientCrewContacts = 6,
    HeatTooHigh = 7,
    NeedsCrewTier = 8,
    NeedsOrganizationTier = 9,
    ActiveWarrant = 10,
    Incarcerated = 11,
    LegalTierRestricted = 12,
    OnProbationOnly = 13,
};

struct StreetCrimeDefinition {
    const char* id;
    const char* displayName;
    const char* description;
    StreetCrimeTier tier;
    CrimeLegalTier legalTier;
    int32_t cooldownTicks;
    int32_t heatOnSuccess;
    int32_t heatOnFailure;
    int32_t baseSuccessPercent;
    int64_t payoutMinCents;
    int64_t payoutMaxCents;
    float minStreetCrimePath;
    float minNetworkAccess;
    int32_t minCriminalTrust;
    int32_t minTrustedCrewContacts;
    int32_t maxHeatToAttempt;
};

struct PlayerStreetCrimeStore {
    uint64_t lastAttemptTickByCrime[MAX_STREET_CRIME_COUNT]{};
};

void resetPlayerStreetCrimeStore(PlayerStreetCrimeStore& store);
int32_t getStreetCrimeCount();
const StreetCrimeDefinition* getStreetCrimeDefinition(int32_t crimeIndex);
int32_t computeBestCriminalContactTrust(const CharacterAgentStore& agentStore);
int32_t countTrustedCriminalContacts(const CharacterAgentStore& agentStore, int32_t minTrust);
StreetCrimeLockReason evaluateStreetCrimeLock(
    const PlayerOperationsStore& operationsStore,
    const PlayerStreetCrimeStore& crimeStore,
    const PlayerLawEnforcementStore& lawStore,
    const PlayerCriminalJusticeStore& justiceStore,
    const PlayerOrganizationStore& organizationStore,
    const PlayerProfile& profile,
    const CharacterAgentStore& agentStore,
    int32_t crimeIndex,
    const StreetCrimeDefinition& crime,
    uint64_t tickCount);
bool tryCommitStreetCrime(
    const PlayerOperationsStore& operationsStore,
    PlayerStreetCrimeStore& crimeStore,
    PlayerLawEnforcementStore& lawStore,
    PlayerCriminalJusticeStore& justiceStore,
    const PlayerOrganizationStore& organizationStore,
    PlayerWallet& wallet,
    CharacterAgentStore& agentStore,
    const PlayerProfile& profile,
    int32_t crimeIndex,
    uint64_t tickCount,
    uint64_t worldSeed);
const char* streetCrimeLockReasonToString(StreetCrimeLockReason reason);
const char* streetCrimeTierToString(StreetCrimeTier tier);

} // namespace Core
