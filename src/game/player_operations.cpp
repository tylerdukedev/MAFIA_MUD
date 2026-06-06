#include "game/player_operations.h"
#include "character/character_social_network.h"
#include "character/character_types.h"
#include "sim/world_event_types.h"
#include "world/landmark_table.h"
#include <algorithm>

namespace Core {

void resetPlayerOperationsStore(PlayerOperationsStore& store) {
    store.headquartersKind = HeadquartersKind::None;
    store.employedBusinessIndices[0] = -1;
    store.employedBusinessIndices[1] = -1;
    store.activeOperationCount = 0;
    store.familyOpinionPenalty = 0;
    store.headquartersEstablishedTick = 0;
    store.lastMonthlyLedgerTick = 0;
    store.lastFamilyUpkeepTick = 0;
    store.headquartersRegionId = 0;
    store.consecutiveUnpaidRentMonths = 0;
    store.rentMultiplierBps = RENT_MULTIPLIER_BPS_BASE;
    store.rentEventAdjustmentBps = 0;
    for (int32_t index = 0; index < MAX_OPERATION_CATALOG_COUNT; ++index) {
        store.activeCatalogIndices[index] = -1;
    }
    store.workExperienceMonths = 0;
    for (int32_t businessIndex = 0; businessIndex < MAX_BUSINESS_NODE_COUNT; ++businessIndex) {
        store.jobReapplyAvailableTickByBusiness[businessIndex] = 0ULL;
    }
    store.homePropertyIndex = -1;
    store.housingTenure = HousingTenure::None;
}

void clearPlayerHeadquarters(PlayerOperationsStore& store) {
    store.headquartersKind = HeadquartersKind::None;
    store.headquartersEstablishedTick = 0;
    store.headquartersRegionId = 0;
    store.lastMonthlyLedgerTick = 0;
    store.lastFamilyUpkeepTick = 0;
    store.consecutiveUnpaidRentMonths = 0;
    store.rentEventAdjustmentBps = 0;
    store.rentMultiplierBps = RENT_MULTIPLIER_BPS_BASE;
    store.homePropertyIndex = -1;
    store.housingTenure = HousingTenure::None;
}

bool hasPlayerHeadquarters(const PlayerOperationsStore& store) {
    return store.headquartersKind != HeadquartersKind::None;
}

bool isOperationCatalogActive(const PlayerOperationsStore& store, int32_t catalogIndex) {
    for (int32_t index = 0; index < store.activeOperationCount; ++index) {
        if (store.activeCatalogIndices[index] == catalogIndex) {
            return true;
        }
    }
    return false;
}

float getNetworkAccessScore(const PlayerProfile& profile) {
    const NetworkAccessProfile& network = profile.networkAccess;
    return std::max(
        {network.ethnicNetwork, network.politicalMachine, network.lawEnforcementChannel, network.businessAssociation, network.importPipeline});
}

int32_t getPlayerReputationScore(const PlayerProfile& profile) {
    const float legitimacy = profile.legitimacy.publicFacingJobAccess + profile.legitimacy.shellCompanyEase;
    const float loyalty = profile.loyaltyBias.kinAlliancePreference + profile.loyaltyBias.ethnicFactionResistance;
    const float normalized = (legitimacy + loyalty) * 0.5f;
    const int32_t reputation = static_cast<int32_t>(normalized * static_cast<float>(MAX_PLAYER_REPUTATION));
    if (reputation < 0) {
        return 0;
    }
    if (reputation > MAX_PLAYER_REPUTATION) {
        return MAX_PLAYER_REPUTATION;
    }
    return reputation;
}

OperationLockReason evaluateOperationLock(
    const PlayerOperationsStore& store,
    const PlayerProfile& profile,
    const PlayerWallet& wallet,
    int32_t catalogIndex,
    const OperationDefinition& operation) {
    if (operation.category == OperationCategory::Headquarters) {
        if (hasPlayerHeadquarters(store)) {
            return OperationLockReason::HeadquartersAlreadySet;
        }
        if (operation.requiresFamilyInCountry && !hasPersonalLodgingOption(profile.draft)) {
            return OperationLockReason::MissingFamilyInCountry;
        }
        if (wallet.cashCents < operation.costCents) {
            return OperationLockReason::InsufficientWealth;
        }
        if (getNetworkAccessScore(profile) < operation.minNetworkAccess) {
            return OperationLockReason::InsufficientNetwork;
        }
        return OperationLockReason::None;
    }
    if (!hasPlayerHeadquarters(store)) {
        return OperationLockReason::NeedsHeadquarters;
    }
    if (isOperationCatalogActive(store, catalogIndex)) {
        return OperationLockReason::AlreadyEstablished;
    }
    if (wallet.cashCents < operation.costCents) {
        return OperationLockReason::InsufficientWealth;
    }
    if (getNetworkAccessScore(profile) < operation.minNetworkAccess) {
        return OperationLockReason::InsufficientNetwork;
    }
    if (getPlayerReputationScore(profile) < operation.minReputation) {
        return OperationLockReason::InsufficientReputation;
    }
    return OperationLockReason::None;
}

bool tryEstablishOperation(
    PlayerOperationsStore& store,
    PlayerWallet& wallet,
    const PlayerProfile& profile,
    int32_t catalogIndex,
    uint64_t tickCount) {
    const OperationDefinition* operation = getOperationDefinition(catalogIndex);
    if (operation == nullptr) {
        return false;
    }
    if (evaluateOperationLock(store, profile, wallet, catalogIndex, *operation) != OperationLockReason::None) {
        return false;
    }
    if (!tryDebitCash(wallet, operation->costCents)) {
        return false;
    }
    if (operation->category == OperationCategory::Headquarters) {
        store.headquartersKind = operation->headquartersKind;
        store.headquartersEstablishedTick = tickCount;
        store.lastMonthlyLedgerTick = 0;
        store.lastFamilyUpkeepTick = 0;
        store.consecutiveUnpaidRentMonths = 0;
        const RegionId regionId = regionIdFromBoroughPreferenceIndex(profile.draft.selectedBoroughIndex);
        store.headquartersRegionId = static_cast<uint8_t>(regionId);
        if (operation->headquartersKind == HeadquartersKind::FamilyFriendDpa) {
            store.familyOpinionPenalty = 8;
        }
        return true;
    }
    if (store.activeOperationCount >= MAX_OPERATION_CATALOG_COUNT) {
        restoreCashWithoutDelta(wallet, operation->costCents);
        return false;
    }
    store.activeCatalogIndices[store.activeOperationCount] = catalogIndex;
    store.activeOperationCount += 1;
    return true;
}

int64_t computeCityEstablishCostCents(const PlayerProfile& profile) {
    if (profile.draft.backgroundId == BackgroundId::StreetHustler) {
        return CITY_ESTABLISH_STREET_HUSTLER_COST_CENTS;
    }
    return CITY_ESTABLISH_COST_CENTS;
}

bool canEstablishCityOperation(
    const PlayerOperationsStore& store,
    const PlayerProfile& profile,
    const PlayerWallet& wallet,
    int64_t& outCostCents) {
    outCostCents = computeCityEstablishCostCents(profile);
    if (!hasPlayerHeadquarters(store)) {
        return false;
    }
    if (wallet.cashCents < outCostCents) {
        return false;
    }
    if (wallet.cashCents < CITY_ESTABLISH_MIN_WEALTH_CENTS) {
        return false;
    }
    if (getNetworkAccessScore(profile) < CITY_ESTABLISH_MIN_NETWORK_ACCESS) {
        return false;
    }
    if (getPlayerReputationScore(profile) < CITY_ESTABLISH_MIN_REPUTATION) {
        return false;
    }
    return true;
}

} // namespace Core
