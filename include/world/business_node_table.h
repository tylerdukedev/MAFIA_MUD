#pragma once

#include "core/types.h"
#include "world/chunk_store.h"
#include <cstdint>

namespace Core {

constexpr int32_t MAX_BUSINESS_NODE_COUNT = 64;

enum class BusinessNodeKind : uint8_t {
    Employer = 0,
    LawOffice = 1,
    RealEstateOffice = 2,
    Bank = 3,
    CarDealershipNew = 4,
    CarDealershipUsed = 5,
};

enum class BusinessIndustry : uint8_t {
    Retail = 0,
    FoodService = 1,
    Logistics = 2,
    Manufacturing = 3,
    Office = 4,
    Hospitality = 5,
    Construction = 6,
};

enum class JobScheduleType : uint8_t {
    FullTime = 0,
    PartTime = 1,
};

enum class BusinessTraitFlags : uint16_t {
    None = 0,
    UnionShop = 1U << 0,
    CashHeavy = 1U << 1,
    NightShift = 1U << 2,
    HighTurnover = 1U << 3,
    StreetFacing = 1U << 4,
    InstitutionalCover = 1U << 5,
};

struct BusinessNodeDefinition {
    const char* id;
    int32_t tileX;
    int32_t tileY;
    const char* fullName;
    const char* mapLabel;
    int64_t jobWageCents;
    float minNetworkAccess;
    BusinessNodeKind kind = BusinessNodeKind::Employer;
    BusinessIndustry industry = BusinessIndustry::Retail;
    uint16_t traitFlags = 0;
    float wageMultiplier = 1.0f;
    uint8_t preferredBackgroundId = 0;
    JobScheduleType scheduleType = JobScheduleType::FullTime;
};

const char* businessIndustryToLabel(BusinessIndustry industry);
const char* businessTraitsToShortLabel(uint16_t traitFlags);
int64_t computeBusinessMonthlyWageCents(const BusinessNodeDefinition& business);

bool isLawOfficeBusinessIndex(int32_t businessIndex);
bool isRealEstateOfficeBusinessIndex(int32_t businessIndex);
bool isBankBusinessIndex(int32_t businessIndex);
bool isCarDealershipBusinessIndex(int32_t businessIndex);

int32_t getBusinessNodeCount();
const BusinessNodeDefinition* getBusinessNodeDefinition(int32_t businessIndex);
int32_t findBusinessNodeIndexAtTile(int32_t tileX, int32_t tileY);
RegionId getBusinessNodeRegionId(int32_t businessIndex);
RegionId getBusinessNodeRegionId(int32_t businessIndex, const ChunkStore& chunkStore);

} // namespace Core
