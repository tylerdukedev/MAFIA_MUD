#pragma once

#include "game/crime_legal_tier.h"
#include "game/criminal_record.h"
#include "game/player_information_feed.h"
#include "game/player_law_enforcement.h"
#include "game/player_organization.h"
#include "game/player_wallet.h"
#include "game/player_world_state.h"
#include "world/chunk_store.h"
#include "world/world_config.h"
#include "sim/character_agent.h"
#include "world/city_control.h"
#include <cstdint>

namespace Core {

struct PlayerLegalCounselStore;

enum class CustodyPhase : uint8_t {
    Free = 0,
    Arrested = 1,
    InJail = 2,
    AwaitingCourt = 3,
    InPrison = 4,
    OnProbation = 5,
    OnParole = 6,
    OnBail = 7,
};

enum class CourtOutcome : uint8_t {
    None = 0,
    Acquitted = 1,
    Probation = 2,
    Prison = 3,
};

constexpr int32_t JUSTICE_ARREST_BOOKING_TICKS = 30;
constexpr int32_t JUSTICE_JAIL_HOLD_TICKS = 35;
constexpr int32_t JUSTICE_PRETRIAL_DELAY_VARIANCE_TICKS = 45;
constexpr int32_t JUSTICE_PRETRIAL_PAROLE_EVIDENCE_THRESHOLD = 24;
constexpr int32_t JUSTICE_SKIP_COURT_WARRANT_COUNT = 1;
constexpr int32_t JUSTICE_SKIP_COURT_HEAT_PENALTY = 14;
constexpr int32_t COURT_APPEARANCE_TILE_X = 252;
constexpr int32_t COURT_APPEARANCE_TILE_Y = 232;
constexpr int32_t JUSTICE_COURT_DELAY_AFTER_BOND_TICKS = 60;
constexpr int32_t JUSTICE_PRISON_BASE_TICKS = 120;
constexpr int32_t JUSTICE_PRISON_PER_LEGAL_TIER_TICKS = 40;
constexpr int32_t JUSTICE_PROBATION_TICKS = 200;
constexpr int32_t JUSTICE_PAROLE_TICKS = 120;
constexpr int32_t JUSTICE_RIVAL_ENCROACH_INTERVAL_TICKS = 80;
constexpr int32_t JUSTICE_ARREST_ROLL_INTERVAL_TICKS = 50;
constexpr int32_t JUSTICE_ARREST_BASE_CHANCE_PERCENT = 6;
constexpr int32_t JUSTICE_ARREST_WARRANT_BONUS_PERCENT = 10;
constexpr int32_t JUSTICE_ARREST_POST_CRIME_CHANCE_PERCENT = 22;
constexpr int32_t JUSTICE_AGENT_ARREST_INTERVAL_TICKS = 120;
constexpr int32_t JUSTICE_AGENT_ARREST_BASE_CHANCE_PERCENT = 4;
constexpr int64_t JUSTICE_BOND_MIN_CENTS = 5000;
constexpr int64_t JUSTICE_BOND_EVIDENCE_CENTS = 450;
constexpr int64_t JUSTICE_BOND_TIER_CENTS = 2500;

struct AgentCustodySlot {
    uint8_t custodyPhase = 0;
    int32_t custodyTicksRemaining = 0;
    uint8_t custodyLegalTier = 0;
};

struct PlayerCriminalJusticeStore {
    uint8_t custodyPhase = 0;
    int32_t phaseTicksRemaining = 0;
    int64_t bondCents = 0;
    uint8_t isBondPosted = 0;
    uint8_t pendingCourtModal = 0;
    uint8_t pendingLegalTier = 0;
    uint8_t lastCourtOutcome = 0;
    int32_t probationTicksRemaining = 0;
    int32_t paroleTicksRemaining = 0;
    int32_t arrestCount = 0;
    uint64_t custodyStartedTick = 0;
    uint64_t courtAppearanceTick = 0;
    uint8_t isPreTrialParoleRelease = 0;
    uint64_t lastRivalEncroachTick = 0;
    uint64_t lastArrestRollTick = 0;
    char lastCustodyLabel[48]{};
    AgentCustodySlot agentCustody[MAX_CHARACTER_AGENT_COUNT]{};
};

void resetPlayerCriminalJusticeStore(PlayerCriminalJusticeStore& store);
CustodyPhase getPlayerCustodyPhase(const PlayerCriminalJusticeStore& store);
bool isPlayerFullyIncarcerated(const PlayerCriminalJusticeStore& store);
bool isPlayerFreeForStreetOperations(const PlayerCriminalJusticeStore& store);
CrimeLegalTier getPlayerMaxCrimeLegalTier(const PlayerCriminalJusticeStore& store);
bool canAttemptCrimeLegalTier(const PlayerCriminalJusticeStore& store, CrimeLegalTier requiredTier);
const char* custodyPhaseToString(CustodyPhase phase);
const char* courtOutcomeToString(CourtOutcome outcome);
int64_t computeArrestBondCents(CrimeLegalTier legalTier, const PlayerLawEnforcementStore& lawStore);
int32_t computePrisonSentenceTicks(CrimeLegalTier legalTier);
void beginPlayerArrest(
    PlayerCriminalJusticeStore& justiceStore,
    PlayerLawEnforcementStore& lawStore,
    CrimeLegalTier legalTier,
    uint64_t tickCount,
    const char* arrestLabel,
    uint64_t worldSeed = 0,
    uint8_t regionId = 1,
    CriminalRecordStore* criminalRecord = nullptr,
    PoliceContactStore* policeContacts = nullptr);
bool tryPayPlayerBond(
    PlayerCriminalJusticeStore& justiceStore,
    PlayerWallet& wallet,
    const PlayerLawEnforcementStore& lawStore,
    const PlayerOrganizationStore& organizationStore,
    const PlayerWorldState& worldState,
    const ChunkStore& chunkStore,
    const WorldConfig& worldConfig,
    uint64_t tickCount);
void commitPlayerCustodyDetention(PlayerCriminalJusticeStore& justiceStore, uint64_t tickCount);
bool shouldReleaseOnPreTrialParole(
    const PlayerCriminalJusticeStore& justiceStore,
    CrimeLegalTier legalTier,
    const PlayerLawEnforcementStore& lawStore,
    const PlayerOrganizationStore& organizationStore);
bool trySkipPlayerCourtDate(
    PlayerCriminalJusticeStore& justiceStore,
    PlayerLawEnforcementStore& lawStore,
    uint64_t tickCount);
void resolvePlayerCourt(
    PlayerCriminalJusticeStore& justiceStore,
    PlayerLawEnforcementStore& lawStore,
    const PlayerLegalCounselStore& legalCounselStore,
    CharacterAgentStore& agentStore,
    uint64_t worldSeed,
    uint64_t tickCount,
    CriminalRecordStore* criminalRecord = nullptr);
void releasePlayerFromCustody(PlayerCriminalJusticeStore& justiceStore, PlayerLawEnforcementStore& lawStore);
void markPlayerCourtModalPending(PlayerCriminalJusticeStore& justiceStore);
void clearPlayerCourtModalPending(PlayerCriminalJusticeStore& justiceStore);
bool isPlayerCourtModalPending(const PlayerCriminalJusticeStore& justiceStore);
bool tryRollPlayerArrest(
    PlayerCriminalJusticeStore& justiceStore,
    PlayerLawEnforcementStore& lawStore,
    CrimeLegalTier legalTier,
    uint64_t worldSeed,
    uint64_t tickCount,
    int32_t extraChancePercent,
    CriminalRecordStore* criminalRecord = nullptr,
    PoliceContactStore* policeContacts = nullptr,
    uint8_t regionId = 1);
void tickPlayerCriminalJustice(
    PlayerCriminalJusticeStore& justiceStore,
    PlayerLawEnforcementStore& lawStore,
    PlayerWallet& wallet,
    CityControlStore& cityControlStore,
    CharacterAgentStore& agentStore,
    uint64_t worldSeed,
    uint64_t tickCount,
    PlayerInformationFeedStore* informationFeedStore = nullptr,
    CriminalRecordStore* criminalRecord = nullptr,
    PoliceContactStore* policeContacts = nullptr,
    uint8_t regionId = 1);
void tickAgentCriminalJustice(
    PlayerCriminalJusticeStore& justiceStore,
    CharacterAgentStore& agentStore,
    const PlayerLawEnforcementStore& lawStore,
    uint64_t worldSeed,
    uint64_t tickCount);
int32_t countPlayerOwnedCitiesForRivalPressure(const CityControlStore& cityControlStore);

} // namespace Core
