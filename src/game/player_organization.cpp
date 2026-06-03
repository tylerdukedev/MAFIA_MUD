#include "game/player_organization.h"
#include "game/player_law_enforcement.h"
#include "game/street_crime.h"
#include "utils/seed_hash.h"
#include <algorithm>
#include <cstring>

namespace Core {

void resetPlayerOrganizationStore(PlayerOrganizationStore& store) {
    store.powerTier = PlayerPowerTier::Solo;
    store.crewMemberCount = 0;
    store.crewName[0] = '\0';
    store.organizationName[0] = '\0';
    store.organizationFront[0] = '\0';
    store.crewFormedTick = 0;
    store.organizationFormedTick = 0;
    for (int32_t index = 0; index < MAX_CREW_MEMBER_COUNT; ++index) {
        store.crewMemberAgentIndices[index] = -1;
    }
}

const char* playerPowerTierToString(PlayerPowerTier tier) {
    switch (tier) {
    case PlayerPowerTier::Crew:
        return "Crew";
    case PlayerPowerTier::Organization:
        return "Organization";
    default:
        return "Solo";
    }
}

int32_t computeAgentLoyaltyScore(const CharacterAgentState& state) {
    return (state.trust + state.respect) / 2;
}

bool isAgentRecruitableForCrew(int32_t agentIndex) {
    return agentIndex == FRIEND_AGENT_SLOT_INDEX || agentIndex == RIVAL_AGENT_SLOT_INDEX;
}

bool isAgentInCrew(const PlayerOrganizationStore& store, int32_t agentIndex) {
    for (int32_t index = 0; index < store.crewMemberCount; ++index) {
        if (store.crewMemberAgentIndices[index] == agentIndex) {
            return true;
        }
    }
    return false;
}

CrewRecruitLockReason evaluateCrewRecruitLock(
    const PlayerOrganizationStore& store,
    const CharacterAgentStore& agentStore,
    int32_t agentIndex) {
    if (store.powerTier == PlayerPowerTier::Organization) {
        return CrewRecruitLockReason::NotSoloOrCrewBuilding;
    }
    if (store.powerTier == PlayerPowerTier::Crew && store.crewMemberCount >= MAX_CREW_MEMBER_COUNT) {
        return CrewRecruitLockReason::CrewFull;
    }
    if (!isAgentRecruitableForCrew(agentIndex)) {
        return CrewRecruitLockReason::NotRecruitableRole;
    }
    if (agentIndex < 0 || agentIndex >= MAX_CHARACTER_AGENT_COUNT) {
        return CrewRecruitLockReason::AgentInactive;
    }
    const CharacterAgentState& state = agentStore.states[agentIndex];
    if (!state.isActive) {
        return CrewRecruitLockReason::AgentInactive;
    }
    if (isAgentInCrew(store, agentIndex)) {
        return CrewRecruitLockReason::AlreadyInCrew;
    }
    if (store.crewMemberCount >= MAX_CREW_MEMBER_COUNT) {
        return CrewRecruitLockReason::CrewFull;
    }
    if (state.opinionOfPlayer < CREW_RECRUIT_MIN_OPINION) {
        return CrewRecruitLockReason::LowOpinion;
    }
    if (state.trust < CREW_RECRUIT_MIN_TRUST) {
        return CrewRecruitLockReason::LowTrust;
    }
    if (state.respect < CREW_RECRUIT_MIN_RESPECT) {
        return CrewRecruitLockReason::LowRespect;
    }
    if (computeAgentLoyaltyScore(state) < CREW_RECRUIT_MIN_LOYALTY_SCORE) {
        return CrewRecruitLockReason::LowLoyalty;
    }
    return CrewRecruitLockReason::None;
}

int32_t rollCrewRecruitAcceptChance(
    const CharacterAgentState& state,
    const PlayerProfile& profile,
    int32_t pitchChoice,
    uint64_t worldSeed,
    int32_t agentIndex) {
    int32_t chance = 42 + state.trust / 4 + state.opinionOfPlayer / 6;
    chance += static_cast<int32_t>(profile.opportunityPaths.streetCrimePath * 18.0f);
    chance += static_cast<int32_t>(profile.loyaltyBias.kinAlliancePreference * 8.0f);
    if (pitchChoice == 0) {
        chance += 8;
    } else if (pitchChoice == 1) {
        chance += 4;
    } else {
        chance -= 6;
    }
    const uint32_t jitter = Utils::hashSeedMix(worldSeed, agentIndex, pitchChoice) % 9U;
    chance += static_cast<int32_t>(jitter) - 4;
    return std::clamp(chance, 18, 88);
}

bool tryAddCrewMember(PlayerOrganizationStore& store, int32_t agentIndex) {
    if (isAgentInCrew(store, agentIndex)) {
        return false;
    }
    if (store.crewMemberCount >= MAX_CREW_MEMBER_COUNT) {
        return false;
    }
    store.crewMemberAgentIndices[store.crewMemberCount] = agentIndex;
    store.crewMemberCount += 1;
    return true;
}

bool tryFormalizeCrew(PlayerOrganizationStore& store, const char* crewNameInput, uint64_t tickCount) {
    if (store.powerTier != PlayerPowerTier::Solo) {
        return false;
    }
    if (store.crewMemberCount < MIN_CREW_MEMBERS_TO_FORM) {
        return false;
    }
    if (crewNameInput != nullptr && crewNameInput[0] != '\0') {
        std::strncpy(store.crewName, crewNameInput, sizeof(store.crewName) - 1);
        store.crewName[sizeof(store.crewName) - 1] = '\0';
    } else {
        std::snprintf(store.crewName, sizeof(store.crewName), "%s", "The Corner Crew");
    }
    store.powerTier = PlayerPowerTier::Crew;
    store.crewFormedTick = tickCount;
    return true;
}

OrganizationFormLockReason evaluateOrganizationFormLock(
    const PlayerOrganizationStore& store,
    const PlayerLawEnforcementStore& lawStore,
    const PlayerProfile& profile,
    const PlayerWallet& wallet) {
    if (store.powerTier != PlayerPowerTier::Crew) {
        return OrganizationFormLockReason::NotCrewTier;
    }
    if (lawStore.activeWarrantCount > 0) {
        return OrganizationFormLockReason::ActiveWarrant;
    }
    if (store.crewMemberCount < MIN_CREW_MEMBERS_TO_FORM) {
        return OrganizationFormLockReason::InsufficientMembers;
    }
    if (getNetworkAccessScore(profile) < ORG_MIN_NETWORK_ACCESS) {
        return OrganizationFormLockReason::InsufficientNetwork;
    }
    if (profile.opportunityPaths.streetCrimePath < ORG_MIN_STREET_CRIME_PATH) {
        return OrganizationFormLockReason::InsufficientStreetPath;
    }
    if (getPlayerReputationScore(profile) < ORG_MIN_REPUTATION_SCORE) {
        return OrganizationFormLockReason::InsufficientReputation;
    }
    if (wallet.lifetimeCrimeCents < ORG_MIN_LIFETIME_CRIME_CENTS) {
        return OrganizationFormLockReason::InsufficientCrimeRecord;
    }
    if (lawStore.personalHeat > 72) {
        return OrganizationFormLockReason::HeatTooHigh;
    }
    return OrganizationFormLockReason::None;
}

bool tryFormalizeOrganization(
    PlayerOrganizationStore& store,
    const char* organizationNameInput,
    const char* organizationFrontInput,
    uint64_t tickCount) {
    if (store.powerTier != PlayerPowerTier::Crew) {
        return false;
    }
    if (organizationNameInput == nullptr || organizationNameInput[0] == '\0') {
        return false;
    }
    std::strncpy(store.organizationName, organizationNameInput, sizeof(store.organizationName) - 1);
    store.organizationName[sizeof(store.organizationName) - 1] = '\0';
    if (organizationFrontInput != nullptr && organizationFrontInput[0] != '\0') {
        std::strncpy(store.organizationFront, organizationFrontInput, sizeof(store.organizationFront) - 1);
        store.organizationFront[sizeof(store.organizationFront) - 1] = '\0';
    } else {
        std::snprintf(store.organizationFront, sizeof(store.organizationFront), "%s", "Import & Storage");
    }
    store.powerTier = PlayerPowerTier::Organization;
    store.organizationFormedTick = tickCount;
    return true;
}

bool meetsStreetCrimeTierRequirement(PlayerPowerTier playerTier, StreetCrimeTier crimeTier) {
    if (crimeTier == StreetCrimeTier::Solo) {
        return true;
    }
    if (crimeTier == StreetCrimeTier::Crew) {
        return playerTier == PlayerPowerTier::Crew || playerTier == PlayerPowerTier::Organization;
    }
    return playerTier == PlayerPowerTier::Organization;
}

} // namespace Core
