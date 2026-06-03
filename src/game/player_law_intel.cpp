#include "game/player_law_intel.h"
#include "character/character_social_network.h"
#include "game/player_wallet.h"
#include <algorithm>
#include <cstring>

namespace Core {

void resetPlayerLawIntelStore(PlayerLawIntelStore& store) {
    store.knownWarrantCount = 0;
    store.warrantIntelMask = 0;
    store.lastBribeAgentIndex = -1;
    store.pendingCovertHeat = 0;
}

int32_t getPlayerKnownWarrantCount(const PlayerLawIntelStore& intelStore, const PlayerLawEnforcementStore& lawStore) {
    if (!doesPlayerKnowAboutWarrants(intelStore, lawStore)) {
        return 0;
    }
    return std::min(intelStore.knownWarrantCount, lawStore.activeWarrantCount);
}

bool doesPlayerKnowAboutWarrants(const PlayerLawIntelStore& intelStore, const PlayerLawEnforcementStore& lawStore) {
    if (lawStore.activeWarrantCount <= 0) {
        return false;
    }
    return intelStore.warrantIntelMask != 0 || intelStore.knownWarrantCount > 0;
}

void revealWarrantsToPlayer(PlayerLawIntelStore& intelStore, const PlayerLawEnforcementStore& lawStore, WarrantIntelSource source) {
    intelStore.knownWarrantCount = lawStore.activeWarrantCount;
    intelStore.warrantIntelMask |= static_cast<uint8_t>(1U << static_cast<uint8_t>(source));
    (void)source;
}

int64_t computePoliceBribeCostCents(int32_t agentIndex, const CharacterAgentState& agentState) {
    int64_t cost = POLICE_BRIBE_BASE_COST_CENTS;
    if (agentIndex == BEAT_COP_AGENT_SLOT_INDEX) {
        cost += POLICE_BRIBE_RANK_COST_SCALE_CENTS;
    } else {
        cost += POLICE_BRIBE_RANK_COST_SCALE_CENTS * 3;
    }
    cost += static_cast<int64_t>(std::max(0, agentState.trust - 40)) * 25LL;
    return cost;
}

bool tryBribePoliceOfficer(
    PlayerLawIntelStore& intelStore,
    PlayerLawEnforcementStore& lawStore,
    CharacterAgentStore& agentStore,
    PlayerWallet& wallet,
    int32_t agentIndex) {
    if (agentIndex != BEAT_COP_AGENT_SLOT_INDEX || !agentStore.states[agentIndex].isActive) {
        return false;
    }
    CharacterAgentState& copState = agentStore.states[agentIndex];
    const int64_t costCents = computePoliceBribeCostCents(agentIndex, copState);
    if (!tryDebitCash(wallet, costCents)) {
        return false;
    }
    const int32_t acceptChance = 35 + copState.trust / 3 + copState.respect / 5;
    const int32_t roll = (copState.trust + copState.respect) % 100;
    if (roll >= acceptChance) {
        adjustAgentOpinion(agentStore, agentIndex, -8);
        return false;
    }
    revealWarrantsToPlayer(intelStore, lawStore, WarrantIntelSource::PoliceBribe);
    intelStore.lastBribeAgentIndex = agentIndex;
    lawStore.evidenceScore = std::max(0, lawStore.evidenceScore - 4);
    adjustAgentOpinion(agentStore, agentIndex, 4);
    return true;
}

bool tryKidnapForIntel(
    PlayerLawIntelStore& intelStore,
    PlayerLawEnforcementStore& lawStore,
    CharacterAgentStore& agentStore,
    int32_t targetAgentIndex,
    uint64_t worldSeed,
    uint64_t tickCount) {
    if (targetAgentIndex < 0 || targetAgentIndex >= MAX_CHARACTER_AGENT_COUNT) {
        return false;
    }
    CharacterAgentState& targetState = agentStore.states[targetAgentIndex];
    if (!targetState.isActive) {
        return false;
    }
    const uint32_t roll = static_cast<uint32_t>((worldSeed ^ tickCount ^ static_cast<uint64_t>(targetAgentIndex)) % 100ULL);
    if (static_cast<int32_t>(roll) > 55) {
        adjustAgentOpinion(agentStore, targetAgentIndex, -20);
        addPlayerHeat(lawStore, KIDNAP_HEAT_PENALTY);
        return false;
    }
    revealWarrantsToPlayer(intelStore, lawStore, WarrantIntelSource::KidnapInformant);
    adjustAgentOpinion(agentStore, targetAgentIndex, -35);
    addPlayerHeat(lawStore, KIDNAP_HEAT_PENALTY / 2);
    return true;
}

bool tryAssassinateTarget(
    PlayerLawIntelStore& intelStore,
    PlayerLawEnforcementStore& lawStore,
    CharacterAgentStore& agentStore,
    int32_t targetAgentIndex,
    bool usedCrewProxy,
    int32_t heatPenalty,
    uint64_t worldSeed,
    uint64_t tickCount) {
    (void)intelStore;
    (void)usedCrewProxy;
    if (targetAgentIndex < 0 || targetAgentIndex >= MAX_CHARACTER_AGENT_COUNT) {
        return false;
    }
    CharacterAgentState& targetState = agentStore.states[targetAgentIndex];
    if (!targetState.isActive) {
        return false;
    }
    const uint32_t roll = static_cast<uint32_t>((worldSeed ^ tickCount ^ 0xA55AULL) % 100ULL);
    int32_t appliedHeat = heatPenalty;
    if (static_cast<int32_t>(roll) < 22) {
        appliedHeat += 12;
    }
    addPlayerHeat(lawStore, appliedHeat);
    targetState.isActive = false;
    lawStore.activeWarrantCount = std::max(0, lawStore.activeWarrantCount - 1);
    return true;
}

} // namespace Core
