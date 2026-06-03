#pragma once

#include "character/player_profile.h"
#include "game/operation_types.h"
#include "game/player_wallet.h"
#include <cstdint>

namespace Core {

struct PlayerOperationsStore {
    HeadquartersKind headquartersKind = HeadquartersKind::None;
    int32_t employedBusinessIndex = -1;
    int32_t activeCatalogIndices[MAX_OPERATION_CATALOG_COUNT]{};
    int32_t activeOperationCount = 0;
    int32_t familyOpinionPenalty = 0;
};

void resetPlayerOperationsStore(PlayerOperationsStore& store);
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
    int32_t catalogIndex);
bool canEstablishCityOperation(
    const PlayerOperationsStore& store,
    const PlayerProfile& profile,
    const PlayerWallet& wallet,
    int64_t& outCostCents);
int64_t computeCityEstablishCostCents(const PlayerProfile& profile);

} // namespace Core
