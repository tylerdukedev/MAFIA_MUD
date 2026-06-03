#include "world/business_node_table.h"
#include <cstring>

namespace Core {

namespace {

constexpr BusinessNodeDefinition BUSINESS_NODE_DEFINITIONS[] = {
    {"mulberry_wholesale", 238, 245, "Mulberry Street Wholesale", "Mulberry Mkt", 325, 0.0f},
    {"bowery_supply", 241, 228, "Bowery Supply House", "Bowery Supply", 300, 0.05f},
    {"harlem_rentals", 268, 195, "Harlem Rooming Office", "Harlem Rooms", 280, 0.0f},
    {"red_hook_docks", 218, 312, "Red Hook Freight Desk", "Red Hook", 360, 0.10f},
    {"bushwick_garment", 305, 296, "Bushwick Garment Loft", "Bushwick Garment", 310, 0.05f},
    {"astoria_bakery", 308, 204, "Astoria Bakery Co-op", "Astoria Bakery", 290, 0.0f},
    {"newark_freight", 115, 176, "Newark Freight Office", "Newark Freight", 340, 0.08f},
    {"jamaica_market", 420, 256, "Jamaica Public Market", "Jamaica Mkt", 295, 0.0f},
    {"bronx_ironworks", 335, 88, "Bronx Ironworks Yard", "Bronx Iron", 330, 0.06f},
    {"staten_boatyard", 104, 352, "Staten Boatyard Office", "Boatyard", 305, 0.04f},
    {"hells_bar_supply", 246, 218, "Hell's Kitchen Bar Supply", "HK Bar Supply", 315, 0.05f},
    {"coney_amusement", 254, 408, "Coney Amusement Office", "Coney Office", 275, 0.0f},
    {"manhattan_law", 252, 232, "Mulberry Street Law Office", "Law Office", 0, 0.0f, BusinessNodeKind::LawOffice},
    {"brooklyn_law", 286, 288, "Bushwick Defense Counsel", "Defense", 0, 0.0f, BusinessNodeKind::LawOffice},
};

constexpr int32_t BUSINESS_NODE_COUNT = static_cast<int32_t>(sizeof(BUSINESS_NODE_DEFINITIONS) / sizeof(BUSINESS_NODE_DEFINITIONS[0]));

constexpr RegionId BUSINESS_NODE_REGION_IDS[] = {
    RegionId::Manhattan, RegionId::Manhattan, RegionId::Manhattan, RegionId::Brooklyn, RegionId::Brooklyn,
    RegionId::Queens, RegionId::NewJersey, RegionId::Queens, RegionId::Bronx, RegionId::StatenIsland,
    RegionId::Manhattan, RegionId::Brooklyn, RegionId::Manhattan, RegionId::Brooklyn,
};

} // namespace

int32_t getBusinessNodeCount() {
    return BUSINESS_NODE_COUNT;
}

const BusinessNodeDefinition* getBusinessNodeDefinition(int32_t businessIndex) {
    if (businessIndex < 0 || businessIndex >= BUSINESS_NODE_COUNT) {
        return nullptr;
    }
    return &BUSINESS_NODE_DEFINITIONS[businessIndex];
}

int32_t findBusinessNodeIndexAtTile(int32_t tileX, int32_t tileY) {
    for (int32_t businessIndex = 0; businessIndex < BUSINESS_NODE_COUNT; ++businessIndex) {
        const BusinessNodeDefinition& business = BUSINESS_NODE_DEFINITIONS[businessIndex];
        if (business.tileX == tileX && business.tileY == tileY) {
            return businessIndex;
        }
    }
    return -1;
}

RegionId getBusinessNodeRegionId(int32_t businessIndex) {
    if (businessIndex < 0 || businessIndex >= BUSINESS_NODE_COUNT) {
        return RegionId::None;
    }
    return BUSINESS_NODE_REGION_IDS[businessIndex];
}

bool isLawOfficeBusinessIndex(int32_t businessIndex) {
    const BusinessNodeDefinition* business = getBusinessNodeDefinition(businessIndex);
    return business != nullptr && business->kind == BusinessNodeKind::LawOffice;
}

} // namespace Core
