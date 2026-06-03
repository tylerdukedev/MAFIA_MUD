#include "game/operation_types.h"
#include <cstring>

namespace Core {

namespace {

constexpr OperationDefinition OPERATION_CATALOG[] = {
    {"hq_rented_room", "Rented Room", OperationCategory::Headquarters, HeadquartersKind::RentedRoom, HQ_RENTED_ROOM_COST_CENTS, 0.0f, 0, 0, false, false},
    {"hq_apartment", "Apartment", OperationCategory::Headquarters, HeadquartersKind::Apartment, HQ_APARTMENT_COST_CENTS, HQ_APARTMENT_MIN_NETWORK_ACCESS, 0, 0, false, false},
    {"hq_family_dpa", "Family / Friend Stay (DPA)", OperationCategory::Headquarters, HeadquartersKind::FamilyFriendDpa, HQ_FAMILY_DPA_COST_CENTS, 0.0f, 0, 0, false, true},
    {"racket_numbers", "Numbers Bank", OperationCategory::Racket, HeadquartersKind::None, 85000, 0.18f, 25000, 20, true, false},
    {"racket_protection", "Protection Ring", OperationCategory::Racket, HeadquartersKind::None, 120000, 0.25f, 40000, 30, true, false},
    {"biz_import_front", "Import Front Company", OperationCategory::Business, HeadquartersKind::None, 200000, 0.35f, 75000, 40, true, false},
    {"biz_nightclub", "Nightclub Front", OperationCategory::Business, HeadquartersKind::None, 175000, 0.28f, 60000, 35, true, false},
    {"log_warehouse", "Warehouse Route", OperationCategory::Logistics, HeadquartersKind::None, 95000, 0.20f, 30000, 25, true, false},
};

constexpr int32_t OPERATION_CATALOG_COUNT = static_cast<int32_t>(sizeof(OPERATION_CATALOG) / sizeof(OPERATION_CATALOG[0]));

} // namespace

int32_t getOperationCatalogCount() {
    return OPERATION_CATALOG_COUNT;
}

const OperationDefinition* getOperationDefinition(int32_t catalogIndex) {
    if (catalogIndex < 0 || catalogIndex >= OPERATION_CATALOG_COUNT) {
        return nullptr;
    }
    return &OPERATION_CATALOG[catalogIndex];
}

int32_t findOperationCatalogIndexById(const char* operationId) {
    if (operationId == nullptr) {
        return -1;
    }
    for (int32_t catalogIndex = 0; catalogIndex < OPERATION_CATALOG_COUNT; ++catalogIndex) {
        if (std::strcmp(OPERATION_CATALOG[catalogIndex].id, operationId) == 0) {
            return catalogIndex;
        }
    }
    return -1;
}

} // namespace Core
