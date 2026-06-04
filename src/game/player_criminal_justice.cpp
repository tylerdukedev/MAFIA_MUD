#include "game/player_criminal_justice.h"
#include "game/legal_counsel.h"
#include "game/game_calendar.h"
#include "game/travel_modes.h"
#include "character/character_social_network.h"
#include "game/economy_constants.h"
#include "game/player_wallet.h"
#include "world/landmark_table.h"
#include "game/agent_relation_events.h"
#include "utils/seed_hash.h"
#include <algorithm>
#include <cstdio>
#include <cstring>

namespace Core {

namespace {

CrimeLegalTier mapCustodyMaxLegalTier(CustodyPhase phase) {
    if (phase == CustodyPhase::OnProbation) {
        return CrimeLegalTier::Street;
    }
    if (phase == CustodyPhase::OnParole) {
        return CrimeLegalTier::Organization;
    }
    return CrimeLegalTier::Financial;
}

CourtOutcome rollCourtOutcome(
    CrimeLegalTier legalTier,
    const PlayerLawEnforcementStore& lawStore,
    const PlayerLegalCounselStore& legalCounselStore,
    uint64_t worldSeed,
    uint64_t tickCount) {
    const uint32_t roll = static_cast<uint32_t>((worldSeed ^ tickCount ^ static_cast<uint64_t>(lawStore.evidenceScore)) % 100ULL);
    const int32_t acquittalBonus = rollCourtAcquittalBonusPercent(legalCounselStore, worldSeed, tickCount);
    if (lawStore.evidenceScore < 12 && static_cast<int32_t>(legalTier) <= static_cast<int32_t>(CrimeLegalTier::Street) && roll < static_cast<uint32_t>(35 + acquittalBonus)) {
        return CourtOutcome::Acquitted;
    }
    if (lawStore.evidenceScore < 32 && roll < 58U) {
        return CourtOutcome::Probation;
    }
    if (static_cast<int32_t>(legalTier) >= static_cast<int32_t>(CrimeLegalTier::Financial) && roll < 85U) {
        return CourtOutcome::Prison;
    }
    if (roll < 42U) {
        return CourtOutcome::Probation;
    }
    return CourtOutcome::Prison;
}

void applyCourtOutcome(
    PlayerCriminalJusticeStore& justiceStore,
    PlayerLawEnforcementStore& lawStore,
    CourtOutcome outcome,
    CrimeLegalTier legalTier,
    uint64_t tickCount) {
    justiceStore.lastCourtOutcome = static_cast<uint8_t>(outcome);
    if (outcome == CourtOutcome::Acquitted) {
        lawStore.evidenceScore = std::max(0, lawStore.evidenceScore - 14);
        lawStore.activeWarrantCount = std::max(0, lawStore.activeWarrantCount - 1);
        lawStore.personalHeat = std::max(PLAYER_HEAT_MIN, lawStore.personalHeat - 10);
        releasePlayerFromCustody(justiceStore, lawStore);
        std::snprintf(justiceStore.lastCustodyLabel, sizeof(justiceStore.lastCustodyLabel), "%s", "Acquitted — case dismissed");
        return;
    }
    if (outcome == CourtOutcome::Probation) {
        justiceStore.custodyPhase = static_cast<uint8_t>(CustodyPhase::OnProbation);
        justiceStore.phaseTicksRemaining = 0;
        justiceStore.probationTicksRemaining = JUSTICE_PROBATION_TICKS;
        lawStore.personalHeat = std::clamp(lawStore.personalHeat + 6, PLAYER_HEAT_MIN, PLAYER_HEAT_MAX);
        std::snprintf(justiceStore.lastCustodyLabel, sizeof(justiceStore.lastCustodyLabel), "%s", "Probation — stay clean");
        return;
    }
    justiceStore.custodyPhase = static_cast<uint8_t>(CustodyPhase::InPrison);
    justiceStore.phaseTicksRemaining = computePrisonSentenceTicks(legalTier);
    justiceStore.custodyStartedTick = tickCount;
    std::snprintf(justiceStore.lastCustodyLabel, sizeof(justiceStore.lastCustodyLabel), "%s", "Sentenced — state time");
}

void tryRivalEncroachPlayerCities(
    PlayerCriminalJusticeStore& justiceStore,
    CityControlStore& cityControlStore,
    const CharacterAgentStore& agentStore,
    uint64_t worldSeed,
    uint64_t tickCount) {
    if (!isPlayerFullyIncarcerated(justiceStore)) {
        return;
    }
    if (justiceStore.lastRivalEncroachTick != 0ULL
        && tickCount - justiceStore.lastRivalEncroachTick < static_cast<uint64_t>(JUSTICE_RIVAL_ENCROACH_INTERVAL_TICKS)) {
        return;
    }
    const CharacterAgentState& rivalState = agentStore.states[RIVAL_AGENT_SLOT_INDEX];
    if (!rivalState.isActive) {
        return;
    }
    justiceStore.lastRivalEncroachTick = tickCount;
    const int32_t ownedCount = countPlayerOwnedCitiesForRivalPressure(cityControlStore);
    if (ownedCount <= 0) {
        return;
    }
    const int32_t encroachChance = 18 + rivalState.respect / 4 + (rivalState.opinionOfPlayer < 0 ? 12 : 0);
    const uint32_t roll = static_cast<uint32_t>((worldSeed ^ tickCount ^ 0x5249564CULL) % 100ULL);
    if (static_cast<int32_t>(roll) >= encroachChance) {
        return;
    }
    for (int32_t landmarkIndex = 0; landmarkIndex < getLandmarkCount(); ++landmarkIndex) {
        if (cityControlStore.slots[landmarkIndex].ownerId != PLAYER_OWNER_ID) {
            continue;
        }
        cityControlStore.slots[landmarkIndex].ownerId = RIVAL_OWNER_ID;
        std::snprintf(justiceStore.lastCustodyLabel, sizeof(justiceStore.lastCustodyLabel), "%s", "Rival seized a claim while you were inside");
        return;
    }
}

void beginAgentArrest(
    PlayerCriminalJusticeStore& justiceStore,
    int32_t agentIndex,
    CrimeLegalTier legalTier,
    int32_t sentenceTicks) {
    AgentCustodySlot& slot = justiceStore.agentCustody[agentIndex];
    slot.custodyPhase = static_cast<uint8_t>(CustodyPhase::InJail);
    slot.custodyTicksRemaining = sentenceTicks;
    slot.custodyLegalTier = static_cast<uint8_t>(legalTier);
}

} // namespace

void resetPlayerCriminalJusticeStore(PlayerCriminalJusticeStore& store) {
    store.custodyPhase = static_cast<uint8_t>(CustodyPhase::Free);
    store.phaseTicksRemaining = 0;
    store.bondCents = 0;
    store.isBondPosted = 0;
    store.pendingCourtModal = 0;
    store.pendingLegalTier = 0;
    store.lastCourtOutcome = 0;
    store.probationTicksRemaining = 0;
    store.paroleTicksRemaining = 0;
    store.arrestCount = 0;
    store.custodyStartedTick = 0;
    store.lastRivalEncroachTick = 0;
    store.courtAppearanceTick = 0;
    store.isPreTrialParoleRelease = 0;
    store.lastArrestRollTick = 0;
    store.lastCustodyLabel[0] = '\0';
    for (int32_t agentIndex = 0; agentIndex < MAX_CHARACTER_AGENT_COUNT; ++agentIndex) {
        store.agentCustody[agentIndex] = AgentCustodySlot{};
    }
}

CustodyPhase getPlayerCustodyPhase(const PlayerCriminalJusticeStore& store) {
    return static_cast<CustodyPhase>(store.custodyPhase);
}

bool isPlayerFullyIncarcerated(const PlayerCriminalJusticeStore& store) {
    const CustodyPhase phase = getPlayerCustodyPhase(store);
    return phase == CustodyPhase::Arrested || phase == CustodyPhase::InJail || phase == CustodyPhase::InPrison;
}

bool isPlayerFreeForStreetOperations(const PlayerCriminalJusticeStore& store) {
    const CustodyPhase phase = getPlayerCustodyPhase(store);
    return phase == CustodyPhase::Free
        || phase == CustodyPhase::OnProbation
        || phase == CustodyPhase::OnParole
        || phase == CustodyPhase::OnBail;
}

CrimeLegalTier getPlayerMaxCrimeLegalTier(const PlayerCriminalJusticeStore& store) {
    const CustodyPhase phase = getPlayerCustodyPhase(store);
    if (phase == CustodyPhase::Free) {
        return CrimeLegalTier::Financial;
    }
    if (isPlayerFullyIncarcerated(store)) {
        return CrimeLegalTier::PettyStreet;
    }
    return mapCustodyMaxLegalTier(phase);
}

bool canAttemptCrimeLegalTier(const PlayerCriminalJusticeStore& store, CrimeLegalTier requiredTier) {
    if (isPlayerFullyIncarcerated(store)) {
        return false;
    }
    return meetsCrimeLegalTierGate(requiredTier, getPlayerMaxCrimeLegalTier(store));
}

const char* custodyPhaseToString(CustodyPhase phase) {
    switch (phase) {
    case CustodyPhase::Arrested:
        return "Arrested — booking";
    case CustodyPhase::InJail:
        return "County jail";
    case CustodyPhase::AwaitingCourt:
        return "Awaiting court";
    case CustodyPhase::InPrison:
        return "State prison";
    case CustodyPhase::OnProbation:
        return "Probation";
    case CustodyPhase::OnParole:
        return "Parole";
    case CustodyPhase::OnBail:
        return "Out on bail";
    default:
        return "Free";
    }
}

const char* courtOutcomeToString(CourtOutcome outcome) {
    switch (outcome) {
    case CourtOutcome::Acquitted:
        return "Acquitted";
    case CourtOutcome::Probation:
        return "Probation";
    case CourtOutcome::Prison:
        return "Prison sentence";
    default:
        return "Pending";
    }
}

int64_t computeArrestBondCents(CrimeLegalTier legalTier, const PlayerLawEnforcementStore& lawStore) {
    int64_t bond = JUSTICE_BOND_MIN_CENTS;
    bond += static_cast<int64_t>(lawStore.evidenceScore) * JUSTICE_BOND_EVIDENCE_CENTS;
    bond += static_cast<int64_t>(static_cast<int32_t>(legalTier)) * JUSTICE_BOND_TIER_CENTS;
    bond += static_cast<int64_t>(lawStore.activeWarrantCount) * 1500LL;
    return bond;
}

int32_t computePrisonSentenceTicks(CrimeLegalTier legalTier) {
    return JUSTICE_PRISON_BASE_TICKS + static_cast<int32_t>(legalTier) * JUSTICE_PRISON_PER_LEGAL_TIER_TICKS;
}

void beginPlayerArrest(
    PlayerCriminalJusticeStore& justiceStore,
    PlayerLawEnforcementStore& lawStore,
    CrimeLegalTier legalTier,
    uint64_t tickCount,
    const char* arrestLabel) {
    if (isPlayerFullyIncarcerated(justiceStore)) {
        return;
    }
    justiceStore.custodyPhase = static_cast<uint8_t>(CustodyPhase::Arrested);
    justiceStore.phaseTicksRemaining = JUSTICE_ARREST_BOOKING_TICKS;
    justiceStore.pendingLegalTier = static_cast<uint8_t>(legalTier);
    justiceStore.bondCents = computeArrestBondCents(legalTier, lawStore);
    justiceStore.isBondPosted = 0;
    justiceStore.custodyStartedTick = tickCount;
    justiceStore.arrestCount += 1;
    lawStore.personalHeat = std::clamp(lawStore.personalHeat + 8, PLAYER_HEAT_MIN, PLAYER_HEAT_MAX);
    refreshInvestigationTier(lawStore);
    if (arrestLabel != nullptr) {
        std::snprintf(justiceStore.lastCustodyLabel, sizeof(justiceStore.lastCustodyLabel), "%s", arrestLabel);
    } else {
        std::snprintf(justiceStore.lastCustodyLabel, sizeof(justiceStore.lastCustodyLabel), "%s", "Arrested — warrant sweep");
    }
}

bool shouldReleaseOnPreTrialParole(
    const PlayerCriminalJusticeStore& justiceStore,
    CrimeLegalTier legalTier,
    const PlayerLawEnforcementStore& lawStore,
    const PlayerOrganizationStore& organizationStore) {
    if (justiceStore.arrestCount >= 2) {
        return true;
    }
    if (static_cast<int32_t>(legalTier) >= static_cast<int32_t>(CrimeLegalTier::Organization)) {
        return true;
    }
    if (lawStore.evidenceScore >= JUSTICE_PRETRIAL_PAROLE_EVIDENCE_THRESHOLD) {
        return true;
    }
    if (organizationStore.powerTier != PlayerPowerTier::Solo) {
        return true;
    }
    return false;
}

void commitPlayerCustodyDetention(PlayerCriminalJusticeStore& justiceStore, uint64_t tickCount) {
    const CustodyPhase phase = getPlayerCustodyPhase(justiceStore);
    if (phase != CustodyPhase::Arrested) {
        return;
    }
    justiceStore.custodyPhase = static_cast<uint8_t>(CustodyPhase::InJail);
    justiceStore.phaseTicksRemaining = JUSTICE_JAIL_HOLD_TICKS;
    justiceStore.custodyStartedTick = tickCount;
    justiceStore.pendingCourtModal = 0;
    std::snprintf(justiceStore.lastCustodyLabel, sizeof(justiceStore.lastCustodyLabel), "%s", "Held in county jail — court soon");
}

bool tryPayPlayerBond(
    PlayerCriminalJusticeStore& justiceStore,
    PlayerWallet& wallet,
    const PlayerLawEnforcementStore& lawStore,
    const PlayerOrganizationStore& organizationStore,
    const PlayerWorldState& worldState,
    const ChunkStore& chunkStore,
    const WorldConfig& worldConfig,
    uint64_t tickCount) {
    if (getPlayerCustodyPhase(justiceStore) != CustodyPhase::Arrested && getPlayerCustodyPhase(justiceStore) != CustodyPhase::InJail) {
        return false;
    }
    if (!tryDebitCash(wallet, justiceStore.bondCents)) {
        return false;
    }
    const CrimeLegalTier legalTier = static_cast<CrimeLegalTier>(justiceStore.pendingLegalTier);
    justiceStore.isBondPosted = 1;
    const int32_t travelLeadHours = computeTravelLeadHours(
        chunkStore,
        worldConfig,
        worldState,
        COURT_APPEARANCE_TILE_X,
        COURT_APPEARANCE_TILE_Y,
        TravelMode::Walk);
    const int32_t courtDelayTicks = JUSTICE_COURT_DELAY_AFTER_BOND_TICKS + travelLeadHours * CALENDAR_TICKS_PER_HOUR;
    justiceStore.courtAppearanceTick = tickCount + static_cast<uint64_t>(courtDelayTicks);
    justiceStore.phaseTicksRemaining = 0;
    justiceStore.pendingCourtModal = 0;
    justiceStore.custodyStartedTick = tickCount;
    justiceStore.isPreTrialParoleRelease = 0;
    if (shouldReleaseOnPreTrialParole(justiceStore, legalTier, lawStore, organizationStore)) {
        justiceStore.custodyPhase = static_cast<uint8_t>(CustodyPhase::OnParole);
        justiceStore.paroleTicksRemaining = JUSTICE_COURT_DELAY_AFTER_BOND_TICKS;
        justiceStore.isPreTrialParoleRelease = 1;
        std::snprintf(justiceStore.lastCustodyLabel, sizeof(justiceStore.lastCustodyLabel), "%s", "Bond posted — pre-trial parole until court");
    } else {
        justiceStore.custodyPhase = static_cast<uint8_t>(CustodyPhase::OnBail);
        std::snprintf(justiceStore.lastCustodyLabel, sizeof(justiceStore.lastCustodyLabel), "%s", "Bond posted — attend court on time");
    }
    return true;
}

bool trySkipPlayerCourtDate(
    PlayerCriminalJusticeStore& justiceStore,
    PlayerLawEnforcementStore& lawStore,
    uint64_t tickCount) {
    (void)tickCount;
    if (getPlayerCustodyPhase(justiceStore) != CustodyPhase::OnBail && getPlayerCustodyPhase(justiceStore) != CustodyPhase::AwaitingCourt) {
        return false;
    }
    lawStore.activeWarrantCount += JUSTICE_SKIP_COURT_WARRANT_COUNT;
    lawStore.personalHeat = std::clamp(lawStore.personalHeat + JUSTICE_SKIP_COURT_HEAT_PENALTY, PLAYER_HEAT_MIN, PLAYER_HEAT_MAX);
    lawStore.evidenceScore = std::min(PLAYER_HEAT_MAX, lawStore.evidenceScore + 8);
    refreshInvestigationTier(lawStore);
    releasePlayerFromCustody(justiceStore, lawStore);
    std::snprintf(justiceStore.lastCustodyLabel, sizeof(justiceStore.lastCustodyLabel), "%s", "Failure to appear — warrant issued");
    return true;
}

void resolvePlayerCourt(
    PlayerCriminalJusticeStore& justiceStore,
    PlayerLawEnforcementStore& lawStore,
    const PlayerLegalCounselStore& legalCounselStore,
    CharacterAgentStore& agentStore,
    uint64_t worldSeed,
    uint64_t tickCount) {
    clearPlayerCourtModalPending(justiceStore);

    // If a witness is on record, check whether a known contact turns state's evidence.
    // Only contacts who are alive, active, and under enough pressure will cooperate.
    if (lawStore.witnessCount > 0) {
        const int32_t candidateSlots[2] = { FRIEND_AGENT_SLOT_INDEX, RIVAL_AGENT_SLOT_INDEX };
        for (int32_t i = 0; i < 2; ++i) {
            const int32_t slot = candidateSlots[i];
            CharacterAgentState& agentState = agentStore.states[slot];
            if (!agentState.isActive) {
                continue;
            }
            if (hasAgentRelationEvent(agentState, AgentRelationEventFlags::SnitchedToPolice)) {
                continue;
            }
            const int32_t pressure = computeAgentSnitchPressure(agentState);
            // Court pressure: under oath with evidence against them, threshold is lower.
            const int32_t courtThreshold = AGENT_SNITCH_PRESSURE_THRESHOLD - 10;
            if (pressure < courtThreshold) {
                continue;
            }
            const uint32_t roll = Utils::hashSeedMix(worldSeed, static_cast<int32_t>(tickCount), slot + 0x434F5254) % 100U;
            if (static_cast<int32_t>(roll) >= pressure) {
                continue;
            }
            // Contact testifies — flag them and add evidence weight.
            markAgentSnitchedToPolice(agentStore, slot);
            lawStore.evidenceScore = std::min(PLAYER_HEAT_MAX, lawStore.evidenceScore + 8);
            break;
        }
    }

    const CrimeLegalTier legalTier = static_cast<CrimeLegalTier>(justiceStore.pendingLegalTier);
    const CourtOutcome outcome = rollCourtOutcome(legalTier, lawStore, legalCounselStore, worldSeed, tickCount);
    applyCourtOutcome(justiceStore, lawStore, outcome, legalTier, tickCount);
    if (outcome == CourtOutcome::Prison) {
        const int32_t reductionPercent = rollPrisonSentenceReductionPercent(legalCounselStore);
        justiceStore.phaseTicksRemaining = justiceStore.phaseTicksRemaining * (100 - reductionPercent) / 100;
        if (justiceStore.phaseTicksRemaining < JUSTICE_PRISON_BASE_TICKS / 2) {
            justiceStore.phaseTicksRemaining = JUSTICE_PRISON_BASE_TICKS / 2;
        }
    }
    refreshInvestigationTier(lawStore);
}

void releasePlayerFromCustody(PlayerCriminalJusticeStore& justiceStore, PlayerLawEnforcementStore& lawStore) {
    (void)lawStore;
    justiceStore.custodyPhase = static_cast<uint8_t>(CustodyPhase::Free);
    justiceStore.phaseTicksRemaining = 0;
    justiceStore.bondCents = 0;
    justiceStore.isBondPosted = 0;
    justiceStore.pendingCourtModal = 0;
    justiceStore.pendingLegalTier = 0;
    justiceStore.courtAppearanceTick = 0;
    justiceStore.isPreTrialParoleRelease = 0;
}

void markPlayerCourtModalPending(PlayerCriminalJusticeStore& justiceStore) {
    justiceStore.pendingCourtModal = 1;
}

void clearPlayerCourtModalPending(PlayerCriminalJusticeStore& justiceStore) {
    justiceStore.pendingCourtModal = 0;
}

bool isPlayerCourtModalPending(const PlayerCriminalJusticeStore& justiceStore) {
    return justiceStore.pendingCourtModal != 0;
}

bool tryRollPlayerArrest(
    PlayerCriminalJusticeStore& justiceStore,
    PlayerLawEnforcementStore& lawStore,
    CrimeLegalTier legalTier,
    uint64_t worldSeed,
    uint64_t tickCount,
    int32_t extraChancePercent) {
    if (isPlayerFullyIncarcerated(justiceStore)) {
        return false;
    }
    int32_t chance = JUSTICE_ARREST_BASE_CHANCE_PERCENT + extraChancePercent;
    if (hasActivePoliceWarrant(lawStore)) {
        chance += JUSTICE_ARREST_WARRANT_BONUS_PERCENT;
    }
    chance += lawStore.personalHeat / 8;
    chance = std::clamp(chance, 2, 55);
    const uint32_t roll = static_cast<uint32_t>((worldSeed ^ tickCount ^ 0x41525253ULL) % 100ULL);
    if (static_cast<int32_t>(roll) >= chance) {
        return false;
    }
    beginPlayerArrest(justiceStore, lawStore, legalTier, tickCount, "Picked up on suspicion");
    return true;
}

void tickPlayerCriminalJustice(
    PlayerCriminalJusticeStore& justiceStore,
    PlayerLawEnforcementStore& lawStore,
    PlayerWallet& wallet,
    CityControlStore& cityControlStore,
    CharacterAgentStore& agentStore,
    uint64_t worldSeed,
    uint64_t tickCount) {
    (void)wallet;
    const CustodyPhase phase = getPlayerCustodyPhase(justiceStore);
    if (phase == CustodyPhase::OnProbation) {
        if (justiceStore.probationTicksRemaining > 0) {
            justiceStore.probationTicksRemaining -= 1;
        }
        if (justiceStore.probationTicksRemaining <= 0) {
            releasePlayerFromCustody(justiceStore, lawStore);
            std::snprintf(justiceStore.lastCustodyLabel, sizeof(justiceStore.lastCustodyLabel), "%s", "Probation complete");
        }
        return;
    }
    if (phase == CustodyPhase::OnParole) {
        if (justiceStore.isPreTrialParoleRelease != 0) {
            if (justiceStore.courtAppearanceTick > 0ULL && tickCount >= justiceStore.courtAppearanceTick) {
                justiceStore.custodyPhase = static_cast<uint8_t>(CustodyPhase::AwaitingCourt);
                justiceStore.pendingCourtModal = 1;
                justiceStore.isPreTrialParoleRelease = 0;
                justiceStore.paroleTicksRemaining = 0;
                std::snprintf(justiceStore.lastCustodyLabel, sizeof(justiceStore.lastCustodyLabel), "%s", "Court date — report now");
            }
            return;
        }
        if (justiceStore.paroleTicksRemaining > 0) {
            justiceStore.paroleTicksRemaining -= 1;
        }
        if (justiceStore.paroleTicksRemaining <= 0) {
            releasePlayerFromCustody(justiceStore, lawStore);
            std::snprintf(justiceStore.lastCustodyLabel, sizeof(justiceStore.lastCustodyLabel), "%s", "Parole complete");
        }
        return;
    }
    if (phase == CustodyPhase::OnBail) {
        if (justiceStore.courtAppearanceTick > 0ULL && tickCount >= justiceStore.courtAppearanceTick) {
            justiceStore.custodyPhase = static_cast<uint8_t>(CustodyPhase::AwaitingCourt);
            justiceStore.pendingCourtModal = 1;
            std::snprintf(justiceStore.lastCustodyLabel, sizeof(justiceStore.lastCustodyLabel), "%s", "Court date — report now");
        }
        return;
    }
    if (phase == CustodyPhase::Free) {
        if (!hasActivePoliceWarrant(lawStore) && lawStore.personalHeat < POLICE_HEAT_WARRANT_THRESHOLD) {
            return;
        }
        if (justiceStore.lastArrestRollTick != 0ULL
            && tickCount - justiceStore.lastArrestRollTick < static_cast<uint64_t>(JUSTICE_ARREST_ROLL_INTERVAL_TICKS)) {
            return;
        }
        justiceStore.lastArrestRollTick = tickCount;
        const CrimeLegalTier tier = lawStore.evidenceScore >= 28 ? CrimeLegalTier::Organization : CrimeLegalTier::Street;
        tryRollPlayerArrest(justiceStore, lawStore, tier, worldSeed, tickCount, 0);
        return;
    }
    if (justiceStore.phaseTicksRemaining > 0) {
        justiceStore.phaseTicksRemaining -= 1;
    }
    if (phase == CustodyPhase::Arrested && justiceStore.phaseTicksRemaining <= 0) {
        if (justiceStore.isBondPosted != 0) {
            justiceStore.custodyPhase = static_cast<uint8_t>(CustodyPhase::AwaitingCourt);
            justiceStore.phaseTicksRemaining = JUSTICE_COURT_DELAY_AFTER_BOND_TICKS;
        } else {
            justiceStore.custodyPhase = static_cast<uint8_t>(CustodyPhase::InJail);
            justiceStore.phaseTicksRemaining = JUSTICE_JAIL_HOLD_TICKS;
            std::snprintf(justiceStore.lastCustodyLabel, sizeof(justiceStore.lastCustodyLabel), "%s", "Held without bond");
        }
    }
    if (phase == CustodyPhase::InJail && justiceStore.phaseTicksRemaining <= 0) {
        justiceStore.custodyPhase = static_cast<uint8_t>(CustodyPhase::AwaitingCourt);
        justiceStore.phaseTicksRemaining = 0;
        justiceStore.pendingCourtModal = 1;
        justiceStore.courtAppearanceTick = tickCount;
        std::snprintf(justiceStore.lastCustodyLabel, sizeof(justiceStore.lastCustodyLabel), "%s", "Transported to court");
    }
    if (getPlayerCustodyPhase(justiceStore) == CustodyPhase::AwaitingCourt
        && justiceStore.pendingCourtModal == 0
        && justiceStore.phaseTicksRemaining <= 0
        && justiceStore.isBondPosted == 0) {
        const PlayerLegalCounselStore defaultCounsel{};
        resolvePlayerCourt(justiceStore, lawStore, defaultCounsel, agentStore, worldSeed, tickCount);
    }
    if (getPlayerCustodyPhase(justiceStore) == CustodyPhase::InPrison) {
        if (justiceStore.phaseTicksRemaining > 0) {
            justiceStore.phaseTicksRemaining -= 1;
        }
        if (justiceStore.phaseTicksRemaining <= 0) {
            justiceStore.custodyPhase = static_cast<uint8_t>(CustodyPhase::OnParole);
            justiceStore.paroleTicksRemaining = JUSTICE_PAROLE_TICKS;
            lawStore.personalHeat = std::clamp(lawStore.personalHeat + 4, PLAYER_HEAT_MIN, PLAYER_HEAT_MAX);
            std::snprintf(justiceStore.lastCustodyLabel, sizeof(justiceStore.lastCustodyLabel), "%s", "Released on parole");
        }
    }
    tryRivalEncroachPlayerCities(justiceStore, cityControlStore, agentStore, worldSeed, tickCount);
}

void tickAgentCriminalJustice(
    PlayerCriminalJusticeStore& justiceStore,
    CharacterAgentStore& agentStore,
    const PlayerLawEnforcementStore& lawStore,
    uint64_t worldSeed,
    uint64_t tickCount) {
    if (tickCount % static_cast<uint64_t>(JUSTICE_AGENT_ARREST_INTERVAL_TICKS) != 0ULL) {
        return;
    }
    for (int32_t agentIndex = FIRST_COMMUNITY_AGENT_SLOT_INDEX; agentIndex < MAX_CHARACTER_AGENT_COUNT; ++agentIndex) {
        CharacterAgentState& agentState = agentStore.states[agentIndex];
        AgentCustodySlot& custody = justiceStore.agentCustody[agentIndex];
        if (!agentState.isActive) {
            custody = AgentCustodySlot{};
            continue;
        }
        if (custody.custodyPhase != static_cast<uint8_t>(CustodyPhase::Free) && custody.custodyTicksRemaining > 0) {
            custody.custodyTicksRemaining -= 1;
            if (custody.custodyTicksRemaining <= 0) {
                custody = AgentCustodySlot{};
            }
            continue;
        }
        if (agentIndex == RIVAL_AGENT_SLOT_INDEX || agentIndex == BEAT_COP_AGENT_SLOT_INDEX) {
            continue;
        }
        int32_t chance = JUSTICE_AGENT_ARREST_BASE_CHANCE_PERCENT + lawStore.personalHeat / 12;
        if (agentState.respect < 25) {
            chance += 4;
        }
        const uint32_t roll = static_cast<uint32_t>((worldSeed ^ tickCount ^ static_cast<uint64_t>(agentIndex)) % 100ULL);
        if (static_cast<int32_t>(roll) >= chance) {
            continue;
        }
        const CrimeLegalTier tier = CrimeLegalTier::Street;
        beginAgentArrest(justiceStore, agentIndex, tier, JUSTICE_JAIL_HOLD_TICKS / 2);
    }
}

int32_t countPlayerOwnedCitiesForRivalPressure(const CityControlStore& cityControlStore) {
    return countPlayerOwnedCities(cityControlStore);
}

} // namespace Core
