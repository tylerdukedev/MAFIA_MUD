#pragma once

#include "character/player_profile.h"
#include "game/crime_legal_tier.h"
#include "game/player_wallet.h"
#include "game/police_contacts.h"

namespace Core {
struct PlayerOrganizationStore;
struct InvestigationCaseStore;
struct EvidenceSystemStore;
}
#include <cstdint>

namespace Core {

constexpr int32_t PLAYER_HEAT_MIN = 0;
constexpr int32_t PLAYER_HEAT_MAX = 100;
constexpr int32_t POLICE_HEAT_DECAY_INTERVAL_TICKS = 10;
constexpr int32_t POLICE_INVESTIGATION_TIER1_HEAT = 18;
constexpr int32_t POLICE_INVESTIGATION_TIER2_HEAT = 40;
constexpr int32_t POLICE_INVESTIGATION_TIER3_HEAT = 65;
constexpr int32_t POLICE_EVIDENCE_WARRANT_THRESHOLD = 24;
constexpr int32_t POLICE_HEAT_WARRANT_THRESHOLD = 28;
constexpr int32_t POLICE_WITNESS_BASE_PERCENT = 10;
constexpr int32_t POLICE_WITNESS_FAIL_BONUS_PERCENT = 14;
constexpr int32_t POLICE_WITNESS_NOTORIETY_SCALE_PERCENT = 22;
constexpr int32_t POLICE_EVIDENCE_DECAY_INTERVAL_TICKS = 40;
constexpr int32_t POLICE_MAX_ACTIVE_WARRANTS = 2;

struct PlayerLawEnforcementStore {
    int32_t personalHeat = 0;
    int32_t investigationTier = 0;
    int32_t evidenceScore = 0;
    int32_t witnessCount = 0;
    int32_t activeWarrantCount = 0;
    uint64_t lastHeatDecayTick = 0;
    uint64_t lastEvidenceDecayTick = 0;
    char lastWitnessLabel[32]{};
};

void resetPlayerLawEnforcementStore(PlayerLawEnforcementStore& store);
void addPlayerHeat(PlayerLawEnforcementStore& store, int32_t heatDelta);
void addPoliceEvidence(PlayerLawEnforcementStore& store, int32_t evidenceDelta);
void refreshInvestigationTier(PlayerLawEnforcementStore& store);
int32_t computePlayerStreetNotoriety(
    const PlayerLawEnforcementStore& lawStore,
    const PlayerOrganizationStore& organizationStore,
    const PlayerProfile& profile,
    const PlayerWallet& wallet);
bool tryIssueWarrantIfThresholdMet(PlayerLawEnforcementStore& store);
void rollStreetCrimeWitness(
    PlayerLawEnforcementStore& store,
    const PlayerOrganizationStore& organizationStore,
    const PlayerProfile& profile,
    const PlayerWallet& wallet,
    uint64_t worldSeed,
    uint64_t tickCount,
    int32_t crimeIndex,
    bool crimeSucceeded);
void tickPoliceEvidenceDecay(PlayerLawEnforcementStore& store, uint64_t tickCount);
void tickPoliceWarrantReview(PlayerLawEnforcementStore& store, uint64_t tickCount);
const char* getPoliceInvestigationLabel(int32_t investigationTier);
bool hasActivePoliceWarrant(const PlayerLawEnforcementStore& store);
void syncLawEnforcementEvidenceRollup(
    PlayerLawEnforcementStore& store,
    const InvestigationCaseStore& caseStore,
    const EvidenceSystemStore& evidenceStore);
void dispatchPoliceOnCrime(
    InvestigationCaseStore& caseStore,
    CrimeLegalTier legalTier,
    uint64_t tickCount,
    const char* dispatchLabel);

} // namespace Core
