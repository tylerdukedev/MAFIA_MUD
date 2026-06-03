#include "game/street_crime.h"
#include "game/economy_constants.h"
#include "game/player_law_enforcement.h"
#include "game/player_organization.h"
#include "game/player_operations.h"
#include "utils/seed_hash.h"
#include <algorithm>
#include <cstring>

namespace Core {

namespace {

constexpr StreetCrimeDefinition STREET_CRIME_DEFINITIONS[] = {
    {"pickpocket", "Pickpocket", "Quick lift on a crowded corner. Solo work.", StreetCrimeTier::Solo, CrimeLegalTier::PettyStreet, 35, 2, 1, 72, 18, 55, 0.0f, 0.0f, 0, 0, PLAYER_HEAT_MAX},
    {"shoplift", "Shoplift", "Snatch goods from an open storefront.", StreetCrimeTier::Solo, CrimeLegalTier::PettyStreet, 45, 3, 2, 68, 28, 85, 0.05f, 0.0f, 0, 0, PLAYER_HEAT_MAX},
    {"shake_vendor", "Shake Down Vendor", "Lean on a pushcart for a few dollars.", StreetCrimeTier::Solo, CrimeLegalTier::Street, 55, 5, 3, 62, 45, 140, 0.12f, 0.0f, 0, 0, 90},
    {"alley_mugging", "Alley Mugging", "One lookout, one muscle — split the take.", StreetCrimeTier::Crew, CrimeLegalTier::Street, 80, 8, 5, 55, 95, 240, 0.20f, 0.0f, 35, 1, 85},
    {"warehouse_pinch", "Warehouse Pinch", "Slip a crate with a trusted runner.", StreetCrimeTier::Crew, CrimeLegalTier::Organization, 100, 10, 6, 50, 140, 320, 0.28f, 0.08f, 45, 1, 80},
    {"numbers_drop", "Numbers Drop", "Run slips for a crew book on commission.", StreetCrimeTier::Organization, CrimeLegalTier::Organization, 120, 12, 8, 45, 220, 480, 0.35f, 0.15f, 50, 2, 75},
    {"protection_cut", "Protection Cut", "Collect weekly from shops you cover.", StreetCrimeTier::Organization, CrimeLegalTier::Organization, 140, 14, 9, 40, 320, 720, 0.42f, 0.22f, 58, 2, 70},
    {"import_skim", "Import Skim", "Divert a shipment with inside help.", StreetCrimeTier::Organization, CrimeLegalTier::Financial, 160, 18, 12, 38, 450, 1100, 0.50f, 0.30f, 65, 2, 65},
};

constexpr int32_t STREET_CRIME_DEFINITION_COUNT = static_cast<int32_t>(sizeof(STREET_CRIME_DEFINITIONS) / sizeof(STREET_CRIME_DEFINITIONS[0]));

bool isCriminalContactSlot(int32_t agentIndex) {
    return agentIndex == FRIEND_AGENT_SLOT_INDEX || agentIndex == RIVAL_AGENT_SLOT_INDEX;
}

int32_t rollSuccessPercent(
    const StreetCrimeDefinition& crime,
    const PlayerProfile& profile,
    const CharacterAgentStore& agentStore,
    const PlayerLawEnforcementStore& lawStore,
    uint64_t worldSeed,
    int32_t crimeIndex,
    uint64_t tickCount) {
    int32_t chance = crime.baseSuccessPercent;
    chance += static_cast<int32_t>(profile.opportunityPaths.streetCrimePath * 18.0f);
    chance += computeBestCriminalContactTrust(agentStore) / 8;
    chance -= lawStore.personalHeat / 6;
    if (crime.tier == StreetCrimeTier::Crew) {
        chance += countTrustedCriminalContacts(agentStore, crime.minCriminalTrust) * 4;
    }
    if (crime.tier == StreetCrimeTier::Organization) {
        chance += static_cast<int32_t>(getNetworkAccessScore(profile) * 12.0f);
    }
    const uint32_t jitter = Utils::hashSeedMix(worldSeed, static_cast<int32_t>(tickCount), crimeIndex) % 7U;
    chance += static_cast<int32_t>(jitter) - 3;
    return std::clamp(chance, 8, 92);
}

int64_t rollPayoutCents(const StreetCrimeDefinition& crime, uint64_t worldSeed, int32_t crimeIndex, uint64_t tickCount) {
    const int64_t span = crime.payoutMaxCents - crime.payoutMinCents + 1;
    if (span <= 0) {
        return crime.payoutMinCents;
    }
    const uint32_t roll = Utils::hashSeedMix(worldSeed, static_cast<int32_t>(tickCount), crimeIndex + 0x504159) % static_cast<uint32_t>(span);
    return crime.payoutMinCents + static_cast<int64_t>(roll);
}

} // namespace

void resetPlayerStreetCrimeStore(PlayerStreetCrimeStore& store) {
    for (int32_t crimeIndex = 0; crimeIndex < MAX_STREET_CRIME_COUNT; ++crimeIndex) {
        store.lastAttemptTickByCrime[crimeIndex] = 0ULL;
    }
}

int32_t getStreetCrimeCount() {
    return STREET_CRIME_DEFINITION_COUNT;
}

const StreetCrimeDefinition* getStreetCrimeDefinition(int32_t crimeIndex) {
    if (crimeIndex < 0 || crimeIndex >= STREET_CRIME_DEFINITION_COUNT) {
        return nullptr;
    }
    return &STREET_CRIME_DEFINITIONS[crimeIndex];
}

int32_t computeBestCriminalContactTrust(const CharacterAgentStore& agentStore) {
    int32_t bestTrust = 0;
    for (int32_t agentIndex = 0; agentIndex < MAX_CHARACTER_AGENT_COUNT; ++agentIndex) {
        if (!isCriminalContactSlot(agentIndex)) {
            continue;
        }
        const CharacterAgentState& state = agentStore.states[agentIndex];
        if (!state.isActive) {
            continue;
        }
        bestTrust = std::max(bestTrust, state.trust);
    }
    return bestTrust;
}

int32_t countTrustedCriminalContacts(const CharacterAgentStore& agentStore, int32_t minTrust) {
    int32_t trustedCount = 0;
    for (int32_t agentIndex = 0; agentIndex < MAX_CHARACTER_AGENT_COUNT; ++agentIndex) {
        if (!isCriminalContactSlot(agentIndex)) {
            continue;
        }
        const CharacterAgentState& state = agentStore.states[agentIndex];
        if (!state.isActive) {
            continue;
        }
        if (state.trust >= minTrust) {
            trustedCount += 1;
        }
    }
    return trustedCount;
}

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
    uint64_t tickCount) {
    if (isPlayerFullyIncarcerated(justiceStore)) {
        return StreetCrimeLockReason::Incarcerated;
    }
    if (!canAttemptCrimeLegalTier(justiceStore, crime.legalTier)) {
        if (getPlayerCustodyPhase(justiceStore) == CustodyPhase::OnProbation) {
            return StreetCrimeLockReason::OnProbationOnly;
        }
        return StreetCrimeLockReason::LegalTierRestricted;
    }
    if (!hasPlayerHeadquarters(operationsStore)) {
        return StreetCrimeLockReason::NeedsHeadquarters;
    }
    if (hasActivePoliceWarrant(lawStore) && crime.tier == StreetCrimeTier::Organization) {
        return StreetCrimeLockReason::ActiveWarrant;
    }
    if (!meetsStreetCrimeTierRequirement(organizationStore.powerTier, crime.tier)) {
        if (crime.tier == StreetCrimeTier::Organization) {
            return StreetCrimeLockReason::NeedsOrganizationTier;
        }
        return StreetCrimeLockReason::NeedsCrewTier;
    }
    if (lawStore.personalHeat > crime.maxHeatToAttempt) {
        return StreetCrimeLockReason::HeatTooHigh;
    }
    if (profile.opportunityPaths.streetCrimePath < crime.minStreetCrimePath) {
        return StreetCrimeLockReason::InsufficientStreetPath;
    }
    if (getNetworkAccessScore(profile) < crime.minNetworkAccess) {
        return StreetCrimeLockReason::InsufficientNetwork;
    }
    if (crime.tier != StreetCrimeTier::Solo) {
        if (computeBestCriminalContactTrust(agentStore) < crime.minCriminalTrust) {
            return StreetCrimeLockReason::InsufficientCriminalTrust;
        }
        if (countTrustedCriminalContacts(agentStore, crime.minCriminalTrust) < crime.minTrustedCrewContacts) {
            return StreetCrimeLockReason::InsufficientCrewContacts;
        }
    }
    const uint64_t lastAttemptTick = crimeStore.lastAttemptTickByCrime[crimeIndex];
    if (lastAttemptTick != 0ULL && tickCount < lastAttemptTick + static_cast<uint64_t>(crime.cooldownTicks)) {
        return StreetCrimeLockReason::OnCooldown;
    }
    return StreetCrimeLockReason::None;
}

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
    uint64_t worldSeed) {
    const StreetCrimeDefinition* crime = getStreetCrimeDefinition(crimeIndex);
    if (crime == nullptr) {
        return false;
    }
    const StreetCrimeLockReason lockReason = evaluateStreetCrimeLock(
        operationsStore, crimeStore, lawStore, justiceStore, organizationStore, profile, agentStore, crimeIndex, *crime, tickCount);
    if (lockReason != StreetCrimeLockReason::None) {
        return false;
    }
    crimeStore.lastAttemptTickByCrime[crimeIndex] = tickCount;
    const int32_t successChance = rollSuccessPercent(*crime, profile, agentStore, lawStore, worldSeed, crimeIndex, tickCount);
    const uint32_t roll = Utils::hashSeedMix(worldSeed, static_cast<int32_t>(tickCount), crimeIndex + 0x535563) % 100U;
    const bool isSuccess = static_cast<int32_t>(roll) < successChance;
    rollStreetCrimeWitness(lawStore, organizationStore, profile, wallet, worldSeed, tickCount, crimeIndex, isSuccess);
    if (isSuccess) {
        const int64_t payoutCents = rollPayoutCents(*crime, worldSeed, crimeIndex, tickCount);
        creditCrimeCash(wallet, payoutCents);
        addPlayerHeat(lawStore, crime->heatOnSuccess);
        tryIssueWarrantIfThresholdMet(lawStore);
        return true;
    }
    addPlayerHeat(lawStore, crime->heatOnFailure);
    tryIssueWarrantIfThresholdMet(lawStore);
    if (hasActivePoliceWarrant(lawStore) || lawStore.personalHeat >= POLICE_HEAT_WARRANT_THRESHOLD) {
        tryRollPlayerArrest(justiceStore, lawStore, crime->legalTier, worldSeed, tickCount, JUSTICE_ARREST_POST_CRIME_CHANCE_PERCENT);
    }
    if (agentStore.states[BEAT_COP_AGENT_SLOT_INDEX].isActive) {
        adjustAgentOpinion(agentStore, BEAT_COP_AGENT_SLOT_INDEX, -2);
    }
    return false;
}

const char* streetCrimeLockReasonToString(StreetCrimeLockReason reason) {
    switch (reason) {
    case StreetCrimeLockReason::None:
        return "Ready";
    case StreetCrimeLockReason::NeedsHeadquarters:
        return "Need a base in the city first";
    case StreetCrimeLockReason::OnCooldown:
        return "Cooling down";
    case StreetCrimeLockReason::InsufficientStreetPath:
        return "Too green for this work";
    case StreetCrimeLockReason::InsufficientNetwork:
        return "Network too thin";
    case StreetCrimeLockReason::InsufficientCriminalTrust:
        return "No trusted criminal contact";
    case StreetCrimeLockReason::InsufficientCrewContacts:
        return "Need more trusted crew";
    case StreetCrimeLockReason::HeatTooHigh:
        return "Police heat too high";
    case StreetCrimeLockReason::NeedsCrewTier:
        return "Formalize a crew first";
    case StreetCrimeLockReason::NeedsOrganizationTier:
        return "Incorporate your organization first";
    case StreetCrimeLockReason::ActiveWarrant:
        return "Active warrant — too hot for this";
    case StreetCrimeLockReason::Incarcerated:
        return "In custody — cannot run jobs";
    case StreetCrimeLockReason::LegalTierRestricted:
        return "Parole / probation tier cap";
    case StreetCrimeLockReason::OnProbationOnly:
        return "Probation — petty street only";
    default:
        return "Locked";
    }
}

const char* streetCrimeTierToString(StreetCrimeTier tier) {
    switch (tier) {
    case StreetCrimeTier::Solo:
        return "Solo";
    case StreetCrimeTier::Crew:
        return "Crew";
    case StreetCrimeTier::Organization:
        return "Organization";
    default:
        return "Unknown";
    }
}

} // namespace Core
