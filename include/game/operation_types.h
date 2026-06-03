#pragma once

#include <cstdint>

namespace Core {

constexpr int32_t MAX_OPERATION_CATALOG_COUNT = 32;
constexpr int32_t MAX_PLAYER_REPUTATION = 100;
constexpr int64_t CITY_ESTABLISH_COST_CENTS = 250000;
constexpr int64_t CITY_ESTABLISH_STREET_HUSTLER_COST_CENTS = 225000;
constexpr int64_t CITY_ESTABLISH_MIN_WEALTH_CENTS = 50000;
constexpr float CITY_ESTABLISH_MIN_NETWORK_ACCESS = 0.22f;
constexpr int32_t CITY_ESTABLISH_MIN_REPUTATION = 15;
constexpr int64_t HQ_RENTED_ROOM_COST_CENTS = 15000;
constexpr int64_t HQ_APARTMENT_COST_CENTS = 45000;
constexpr int64_t HQ_FAMILY_DPA_COST_CENTS = 0;
constexpr float HQ_APARTMENT_MIN_NETWORK_ACCESS = 0.12f;
constexpr int64_t JOB_WAGE_CENTS_PER_PAYOUT = 350;
constexpr int32_t JOB_PAYOUT_INTERVAL_TICKS = 20;

enum class OperationCategory : uint8_t {
    Headquarters = 0,
    Racket = 1,
    Business = 2,
    Logistics = 3,
};

enum class HeadquartersKind : uint8_t {
    None = 0,
    RentedRoom = 1,
    Apartment = 2,
    FamilyFriendDpa = 3,
};

enum class OperationLockReason : uint8_t {
    None = 0,
    NeedsHeadquarters = 1,
    InsufficientWealth = 2,
    InsufficientNetwork = 3,
    InsufficientReputation = 4,
    MissingFamilyInCountry = 5,
    AlreadyEstablished = 6,
    HeadquartersAlreadySet = 7,
};

struct OperationDefinition {
    const char* id;
    const char* displayName;
    OperationCategory category;
    HeadquartersKind headquartersKind;
    int64_t costCents;
    float minNetworkAccess;
    int64_t minWealthCents;
    int32_t minReputation;
    bool requiresHeadquarters;
    bool requiresFamilyInCountry;
};

int32_t getOperationCatalogCount();
const OperationDefinition* getOperationDefinition(int32_t catalogIndex);
int32_t findOperationCatalogIndexById(const char* operationId);

} // namespace Core
