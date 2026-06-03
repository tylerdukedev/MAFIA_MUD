#pragma once

#include "game/player_law_enforcement.h"
#include "sim/character_agent.h"
#include <cstdint>

namespace Core {

enum class WarrantIntelSource : uint8_t {
    None = 0,
    PoliceBribe = 1,
    KidnapInformant = 2,
    AssassinationCoverup = 3,
    CrewInformant = 4,
};

enum class CovertActionKind : uint8_t {
    BribePolice = 0,
    KidnapTarget = 1,
    AssassinateTarget = 2,
};

constexpr int32_t POLICE_BRIBE_BASE_COST_CENTS = 2500;
constexpr int32_t POLICE_BRIBE_RANK_COST_SCALE_CENTS = 1800;
constexpr int32_t KIDNAP_HEAT_PENALTY = 18;
constexpr int32_t ASSASSINATE_HEAT_PENALTY = 35;

struct PlayerLawIntelStore {
    int32_t knownWarrantCount = 0;
    uint8_t warrantIntelMask = 0;
    int32_t lastBribeAgentIndex = -1;
    int32_t pendingCovertHeat = 0;
};

void resetPlayerLawIntelStore(PlayerLawIntelStore& store);
int32_t getPlayerKnownWarrantCount(const PlayerLawIntelStore& intelStore, const PlayerLawEnforcementStore& lawStore);
bool doesPlayerKnowAboutWarrants(const PlayerLawIntelStore& intelStore, const PlayerLawEnforcementStore& lawStore);
void revealWarrantsToPlayer(PlayerLawIntelStore& intelStore, const PlayerLawEnforcementStore& lawStore, WarrantIntelSource source);
int64_t computePoliceBribeCostCents(int32_t agentIndex, const CharacterAgentState& agentState);
bool tryBribePoliceOfficer(
    PlayerLawIntelStore& intelStore,
    PlayerLawEnforcementStore& lawStore,
    CharacterAgentStore& agentStore,
    PlayerWallet& wallet,
    int32_t agentIndex);
bool tryKidnapForIntel(
    PlayerLawIntelStore& intelStore,
    PlayerLawEnforcementStore& lawStore,
    CharacterAgentStore& agentStore,
    int32_t targetAgentIndex,
    uint64_t worldSeed,
    uint64_t tickCount);
bool tryAssassinateTarget(
    PlayerLawIntelStore& intelStore,
    PlayerLawEnforcementStore& lawStore,
    CharacterAgentStore& agentStore,
    int32_t targetAgentIndex,
    bool usedCrewProxy,
    int32_t heatPenalty,
    uint64_t worldSeed,
    uint64_t tickCount);

} // namespace Core
