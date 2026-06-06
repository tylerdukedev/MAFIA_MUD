#pragma once

#include "character/player_profile.h"
#include "game/operation_types.h"
#include "game/player_wallet.h"
#include "world/business_node_table.h"
#include <cstdint>

namespace Core {

enum class HousingTenure : uint8_t {
    None = 0,
    Rent = 1,
    Own = 2,
};

struct PlayerOperationsStore {
    HeadquartersKind headquartersKind = HeadquartersKind::None;
    int32_t employedBusinessIndices[2] = {-1, -1};
    int32_t activeCatalogIndices[MAX_OPERATION_CATALOG_COUNT]{};
    int32_t activeOperationCount = 0;
    int32_t familyOpinionPenalty = 0;
    uint64_t headquartersEstablishedTick = 0;
    uint64_t lastMonthlyLedgerTick = 0;
    uint64_t lastFamilyUpkeepTick = 0;
    uint8_t headquartersRegionId = 0;
    int8_t consecutiveUnpaidRentMonths = 0;
    int32_t rentMultiplierBps = 10000;
    int32_t rentEventAdjustmentBps = 0;
    int32_t workExperienceMonths = 0;
    uint64_t jobReapplyAvailableTickByBusiness[MAX_BUSINESS_NODE_COUNT]{};
    int32_t homePropertyIndex = -1;
    HousingTenure housingTenure = HousingTenure::None;
};

void resetPlayerOperationsStore(PlayerOperationsStore& store);
void clearPlayerHeadquarters(PlayerOperationsStore& store);
bool hasPlayerHeadquarters(const PlayerOperationsStore& store);
bool isOperationCatalogActive(const PlayerOperationsStore& store, int32_t catalogIndex);
float getNetworkAccessScore(const PlayerProfile& profile);
int32_t getPlayerReputationScore(const PlayerProfile& profile);
OperationLockReason evaluateOperationLock(
    const PlayerOperationsStore& store,
    const PlayerProfile& profile,
    const PlayerWallet& wallet,
    int32_t catalogIndex,
    const OperationDefinition& operation);
bool tryEstablishOperation(
    PlayerOperationsStore& store,
    PlayerWallet& wallet,
    const PlayerProfile& profile,
    int32_t catalogIndex,
    uint64_t tickCount);
bool canEstablishCityOperation(
    const PlayerOperationsStore& store,
    const PlayerProfile& profile,
    const PlayerWallet& wallet,
    int64_t& outCostCents);
int64_t computeCityEstablishCostCents(const PlayerProfile& profile);

} // namespace Core
