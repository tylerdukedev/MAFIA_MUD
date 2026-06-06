#include "world/business_node_table.h"
#include "character/character_types.h"
#include <cstdio>
#include <cstring>

namespace Core {

namespace {

constexpr uint16_t TRAIT_UNION = static_cast<uint16_t>(BusinessTraitFlags::UnionShop);
constexpr uint16_t TRAIT_CASH = static_cast<uint16_t>(BusinessTraitFlags::CashHeavy);
constexpr uint16_t TRAIT_NIGHT = static_cast<uint16_t>(BusinessTraitFlags::NightShift);
constexpr uint16_t TRAIT_TURNOVER = static_cast<uint16_t>(BusinessTraitFlags::HighTurnover);
constexpr uint16_t TRAIT_STREET = static_cast<uint16_t>(BusinessTraitFlags::StreetFacing);
constexpr uint16_t TRAIT_COVER = static_cast<uint16_t>(BusinessTraitFlags::InstitutionalCover);

constexpr BusinessNodeDefinition BUSINESS_NODE_DEFINITIONS[] = {
    {"mulberry_wholesale", 238, 245, "Mulberry Street Wholesale", "Mulberry Mkt", 325, 0.0f, BusinessNodeKind::Employer, BusinessIndustry::Retail, TRAIT_STREET, 1.0f, 0, JobScheduleType::FullTime},
    {"bowery_supply", 241, 228, "Bowery Supply House", "Bowery Supply", 300, 0.05f, BusinessNodeKind::Employer, BusinessIndustry::Logistics, TRAIT_CASH, 1.05f, 0, JobScheduleType::PartTime},
    {"harlem_rentals", 268, 195, "Harlem Rooming Office", "Harlem Rooms", 280, 0.0f, BusinessNodeKind::Employer, BusinessIndustry::Office, TRAIT_COVER, 0.95f, static_cast<uint8_t>(BackgroundId::NeighborhoodOrganizer), JobScheduleType::PartTime},
    {"red_hook_docks", 218, 312, "Red Hook Freight Desk", "Red Hook", 360, 0.10f, BusinessNodeKind::Employer, BusinessIndustry::Logistics, TRAIT_UNION | TRAIT_NIGHT, 1.15f, 0, JobScheduleType::PartTime},
    {"bushwick_garment", 305, 296, "Bushwick Garment Loft", "Bushwick Garment", 310, 0.05f, BusinessNodeKind::Employer, BusinessIndustry::Manufacturing, TRAIT_UNION | TRAIT_TURNOVER, 1.08f, 0, JobScheduleType::FullTime},
    {"astoria_bakery", 308, 204, "Astoria Bakery Co-op", "Astoria Bakery", 290, 0.0f, BusinessNodeKind::Employer, BusinessIndustry::FoodService, TRAIT_STREET, 0.92f, 0, JobScheduleType::PartTime},
    {"newark_freight", 115, 176, "Newark Freight Office", "Newark Freight", 340, 0.08f, BusinessNodeKind::Employer, BusinessIndustry::Logistics, TRAIT_CASH | TRAIT_NIGHT, 1.12f, 0, JobScheduleType::PartTime},
    {"jamaica_market", 420, 256, "Jamaica Public Market", "Jamaica Mkt", 295, 0.0f, BusinessNodeKind::Employer, BusinessIndustry::Retail, TRAIT_STREET | TRAIT_TURNOVER, 1.0f, static_cast<uint8_t>(BackgroundId::StreetHustler), JobScheduleType::PartTime},
    {"bronx_ironworks", 335, 88, "Bronx Ironworks Yard", "Bronx Iron", 330, 0.06f, BusinessNodeKind::Employer, BusinessIndustry::Manufacturing, TRAIT_UNION | TRAIT_CASH, 1.18f, 0, JobScheduleType::FullTime},
    {"staten_boatyard", 104, 352, "Staten Boatyard Office", "Boatyard", 305, 0.04f, BusinessNodeKind::Employer, BusinessIndustry::Construction, TRAIT_NIGHT, 1.02f, 0, JobScheduleType::PartTime},
    {"hells_bar_supply", 246, 218, "Hell's Kitchen Bar Supply", "HK Bar Supply", 315, 0.05f, BusinessNodeKind::Employer, BusinessIndustry::Hospitality, TRAIT_CASH | TRAIT_STREET, 1.06f, static_cast<uint8_t>(BackgroundId::StreetHustler), JobScheduleType::FullTime},
    {"coney_amusement", 254, 408, "Coney Amusement Office", "Coney Office", 275, 0.0f, BusinessNodeKind::Employer, BusinessIndustry::Hospitality, TRAIT_TURNOVER | TRAIT_STREET, 0.88f, 0, JobScheduleType::PartTime},
    {"chinatown_import", 244, 240, "Chinatown Import Ledger", "Import Ledger", 335, 0.12f, BusinessNodeKind::Employer, BusinessIndustry::Logistics, TRAIT_COVER | TRAIT_CASH, 1.14f, static_cast<uint8_t>(BackgroundId::Bookkeeper), JobScheduleType::FullTime},
    {"midtown_typing", 248, 210, "Midtown Typing Pool", "Typing Pool", 320, 0.15f, BusinessNodeKind::Employer, BusinessIndustry::Office, TRAIT_COVER, 1.05f, static_cast<uint8_t>(BackgroundId::Bookkeeper), JobScheduleType::FullTime},
    {"queens_auto", 395, 240, "Queens Auto Body Office", "Auto Body", 340, 0.04f, BusinessNodeKind::Employer, BusinessIndustry::Manufacturing, TRAIT_CASH, 1.1f, 0, JobScheduleType::FullTime},
    {"flushing_grocery", 378, 220, "Flushing Grocery Cooperative", "Flushing Groc", 285, 0.0f, BusinessNodeKind::Employer, BusinessIndustry::Retail, TRAIT_STREET, 0.98f, 0, JobScheduleType::PartTime},
    {"williamsburg_cafe", 278, 268, "Williamsburg Corner Cafe", "Corner Cafe", 270, 0.02f, BusinessNodeKind::Employer, BusinessIndustry::FoodService, TRAIT_TURNOVER | TRAIT_STREET, 0.9f, 0, JobScheduleType::PartTime},
    {"bedford_courier", 300, 285, "Bedford Courier Dispatch", "Courier", 305, 0.03f, BusinessNodeKind::Employer, BusinessIndustry::Logistics, TRAIT_NIGHT | TRAIT_STREET, 1.0f, static_cast<uint8_t>(BackgroundId::StreetHustler), JobScheduleType::PartTime},
    {"washington_heights_clinic", 290, 102, "Washington Heights Clinic Desk", "Clinic Desk", 350, 0.18f, BusinessNodeKind::Employer, BusinessIndustry::Office, TRAIT_COVER, 1.2f, static_cast<uint8_t>(BackgroundId::NeighborhoodOrganizer), JobScheduleType::FullTime},
    {"port_richmond_wharf", 98, 348, "Port Richmond Wharf Clerk", "Wharf Clerk", 325, 0.05f, BusinessNodeKind::Employer, BusinessIndustry::Logistics, TRAIT_UNION, 1.07f, 0, JobScheduleType::FullTime},
    {"soho_gallery", 234, 228, "SoHo Gallery Attendant Office", "Gallery", 310, 0.20f, BusinessNodeKind::Employer, BusinessIndustry::Hospitality, TRAIT_COVER, 1.04f, static_cast<uint8_t>(BackgroundId::Bookkeeper), JobScheduleType::FullTime},
    {"hunts_point_produce", 345, 128, "Hunts Point Produce Shed", "Produce", 355, 0.07f, BusinessNodeKind::Employer, BusinessIndustry::Retail, TRAIT_NIGHT | TRAIT_UNION, 1.16f, 0, JobScheduleType::FullTime},
    {"manhattan_law", 252, 232, "Mulberry Street Law Office", "Law Office", 0, 0.0f, BusinessNodeKind::LawOffice, BusinessIndustry::Office, 0, 1.0f, 0, JobScheduleType::FullTime},
    {"brooklyn_law", 286, 288, "Bushwick Defense Counsel", "Defense", 0, 0.0f, BusinessNodeKind::LawOffice, BusinessIndustry::Office, 0, 1.0f, 0, JobScheduleType::FullTime},
    {"manhattan_re", 265, 177, "Upper East Realty", "UE Realty", 0, 0.0f, BusinessNodeKind::RealEstateOffice, BusinessIndustry::Office, TRAIT_COVER, 1.0f, 0, JobScheduleType::FullTime},
    {"manhattan_bank", 239, 221, "Lexington Trust Branch", "Lex Bank", 0, 0.0f, BusinessNodeKind::Bank, BusinessIndustry::Office, TRAIT_COVER, 1.0f, 0, JobScheduleType::FullTime},
    {"manhattan_cars_new", 227, 205, "Midtown Motors", "Mid Motors", 0, 0.0f, BusinessNodeKind::CarDealershipNew, BusinessIndustry::Retail, TRAIT_STREET, 1.0f, 0, JobScheduleType::FullTime},
    {"manhattan_cars_used", 256, 218, "Hell's Kitchen Auto Lot", "HK Autos", 0, 0.0f, BusinessNodeKind::CarDealershipUsed, BusinessIndustry::Retail, TRAIT_STREET, 1.0f, 0, JobScheduleType::FullTime},
    {"brooklyn_re", 264, 358, "Flatbush Realty", "Flatbush RE", 0, 0.0f, BusinessNodeKind::RealEstateOffice, BusinessIndustry::Office, TRAIT_COVER, 1.0f, 0, JobScheduleType::FullTime},
    {"brooklyn_bank", 219, 318, "Red Hook Savings", "RH Bank", 0, 0.0f, BusinessNodeKind::Bank, BusinessIndustry::Office, TRAIT_COVER, 1.0f, 0, JobScheduleType::FullTime},
    {"brooklyn_cars_new", 275, 272, "Williamsburg Auto Gallery", "WB Autos", 0, 0.0f, BusinessNodeKind::CarDealershipNew, BusinessIndustry::Retail, TRAIT_STREET, 1.0f, 0, JobScheduleType::FullTime},
    {"brooklyn_cars_used", 248, 370, "Bensonhurst Used Cars", "Ben Used", 0, 0.0f, BusinessNodeKind::CarDealershipUsed, BusinessIndustry::Retail, TRAIT_STREET, 1.0f, 0, JobScheduleType::FullTime},
    {"queens_re", 365, 198, "Astoria Property Group", "Astoria RE", 0, 0.0f, BusinessNodeKind::RealEstateOffice, BusinessIndustry::Office, TRAIT_COVER, 1.0f, 0, JobScheduleType::FullTime},
    {"queens_bank", 422, 260, "Jamaica Federal", "Jam Bank", 0, 0.0f, BusinessNodeKind::Bank, BusinessIndustry::Office, TRAIT_COVER, 1.0f, 0, JobScheduleType::FullTime},
    {"queens_cars_new", 404, 235, "Flushing Import Motors", "Flush New", 0, 0.0f, BusinessNodeKind::CarDealershipNew, BusinessIndustry::Retail, TRAIT_STREET, 1.0f, 0, JobScheduleType::FullTime},
    {"queens_cars_used", 388, 228, "Queens Pre-Owned", "Q Used", 0, 0.0f, BusinessNodeKind::CarDealershipUsed, BusinessIndustry::Retail, TRAIT_STREET, 1.0f, 0, JobScheduleType::FullTime},
    {"bronx_re", 338, 112, "Morris Park Realty", "MP Realty", 0, 0.0f, BusinessNodeKind::RealEstateOffice, BusinessIndustry::Office, TRAIT_COVER, 1.0f, 0, JobScheduleType::FullTime},
    {"bronx_bank", 375, 68, "East Bronx Credit Union", "EB Bank", 0, 0.0f, BusinessNodeKind::Bank, BusinessIndustry::Office, TRAIT_COVER, 1.0f, 0, JobScheduleType::FullTime},
    {"bronx_cars_new", 352, 82, "Bronx Avenue Motors", "Bx New", 0, 0.0f, BusinessNodeKind::CarDealershipNew, BusinessIndustry::Retail, TRAIT_STREET, 1.0f, 0, JobScheduleType::FullTime},
    {"bronx_cars_used", 320, 105, "Yankee Used Auto", "Bx Used", 0, 0.0f, BusinessNodeKind::CarDealershipUsed, BusinessIndustry::Retail, TRAIT_STREET, 1.0f, 0, JobScheduleType::FullTime},
    {"staten_re", 107, 385, "Mid Island Realty", "MI Realty", 0, 0.0f, BusinessNodeKind::RealEstateOffice, BusinessIndustry::Office, TRAIT_COVER, 1.0f, 0, JobScheduleType::FullTime},
    {"staten_bank", 92, 440, "Great Kills Trust", "GK Bank", 0, 0.0f, BusinessNodeKind::Bank, BusinessIndustry::Office, TRAIT_COVER, 1.0f, 0, JobScheduleType::FullTime},
    {"staten_cars_new", 145, 416, "Midland Motors", "MI New", 0, 0.0f, BusinessNodeKind::CarDealershipNew, BusinessIndustry::Retail, TRAIT_STREET, 1.0f, 0, JobScheduleType::FullTime},
    {"staten_cars_used", 88, 460, "Eltingville Auto Exchange", "Elt Used", 0, 0.0f, BusinessNodeKind::CarDealershipUsed, BusinessIndustry::Retail, TRAIT_STREET, 1.0f, 0, JobScheduleType::FullTime},
};

constexpr int32_t BUSINESS_NODE_COUNT = static_cast<int32_t>(sizeof(BUSINESS_NODE_DEFINITIONS) / sizeof(BUSINESS_NODE_DEFINITIONS[0]));

constexpr RegionId BUSINESS_NODE_REGION_IDS[] = {
    RegionId::Manhattan, RegionId::Manhattan, RegionId::Manhattan, RegionId::Brooklyn, RegionId::Brooklyn,
    RegionId::Queens, RegionId::NewJersey, RegionId::Queens, RegionId::Bronx, RegionId::StatenIsland,
    RegionId::Manhattan, RegionId::Brooklyn, RegionId::Manhattan, RegionId::Manhattan, RegionId::Queens,
    RegionId::Queens, RegionId::Queens, RegionId::Brooklyn, RegionId::Bronx,
    RegionId::StatenIsland, RegionId::Manhattan, RegionId::Bronx, RegionId::Manhattan, RegionId::Brooklyn,
    RegionId::Manhattan, RegionId::Manhattan, RegionId::Manhattan, RegionId::Manhattan,
    RegionId::Brooklyn, RegionId::Brooklyn, RegionId::Brooklyn, RegionId::Brooklyn,
    RegionId::Queens, RegionId::Queens, RegionId::Queens, RegionId::Queens,
    RegionId::Bronx, RegionId::Bronx, RegionId::Bronx, RegionId::Bronx,
    RegionId::StatenIsland, RegionId::StatenIsland, RegionId::StatenIsland, RegionId::StatenIsland,
};

static_assert(
    sizeof(BUSINESS_NODE_REGION_IDS) / sizeof(BUSINESS_NODE_REGION_IDS[0]) == sizeof(BUSINESS_NODE_DEFINITIONS) / sizeof(BUSINESS_NODE_DEFINITIONS[0]),
    "Business region table must match business definition count");

} // namespace

const char* businessIndustryToLabel(BusinessIndustry industry) {
    switch (industry) {
    case BusinessIndustry::Retail:
        return "Retail";
    case BusinessIndustry::FoodService:
        return "Food service";
    case BusinessIndustry::Logistics:
        return "Logistics";
    case BusinessIndustry::Manufacturing:
        return "Manufacturing";
    case BusinessIndustry::Office:
        return "Office";
    case BusinessIndustry::Hospitality:
        return "Hospitality";
    case BusinessIndustry::Construction:
        return "Construction";
    default:
        return "Workplace";
    }
}

const char* businessTraitsToShortLabel(uint16_t traitFlags) {
    static char labelBuffer[128];
    labelBuffer[0] = '\0';
    if ((traitFlags & TRAIT_UNION) != 0) {
        std::strncat(labelBuffer, "Union shop; ", sizeof(labelBuffer) - 1);
    }
    if ((traitFlags & TRAIT_CASH) != 0) {
        std::strncat(labelBuffer, "Cash-heavy; ", sizeof(labelBuffer) - 1);
    }
    if ((traitFlags & TRAIT_NIGHT) != 0) {
        std::strncat(labelBuffer, "Night shift; ", sizeof(labelBuffer) - 1);
    }
    if ((traitFlags & TRAIT_TURNOVER) != 0) {
        std::strncat(labelBuffer, "High turnover; ", sizeof(labelBuffer) - 1);
    }
    if ((traitFlags & TRAIT_STREET) != 0) {
        std::strncat(labelBuffer, "Street-facing; ", sizeof(labelBuffer) - 1);
    }
    if ((traitFlags & TRAIT_COVER) != 0) {
        std::strncat(labelBuffer, "Institutional cover; ", sizeof(labelBuffer) - 1);
    }
    if (labelBuffer[0] == '\0') {
        std::snprintf(labelBuffer, sizeof(labelBuffer), "Standard workplace");
    }
    return labelBuffer;
}

int64_t computeBusinessMonthlyWageCents(const BusinessNodeDefinition& business) {
    const float scaled = static_cast<float>(business.jobWageCents) * business.wageMultiplier;
    return static_cast<int64_t>(scaled < 1.0f ? 1.0f : scaled);
}

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

RegionId getBusinessNodeRegionId(int32_t businessIndex, const ChunkStore& chunkStore) {
    const BusinessNodeDefinition* business = getBusinessNodeDefinition(businessIndex);
    if (business == nullptr) {
        return RegionId::None;
    }
    const WorldCoord coord{business->tileX, business->tileY};
    const RegionId sampledRegion = chunkStore.getRegionAt(coord);
    if (sampledRegion != RegionId::None) {
        return sampledRegion;
    }
    return getBusinessNodeRegionId(businessIndex);
}

bool isLawOfficeBusinessIndex(int32_t businessIndex) {
    const BusinessNodeDefinition* business = getBusinessNodeDefinition(businessIndex);
    return business != nullptr && business->kind == BusinessNodeKind::LawOffice;
}

bool isRealEstateOfficeBusinessIndex(int32_t businessIndex) {
    const BusinessNodeDefinition* business = getBusinessNodeDefinition(businessIndex);
    return business != nullptr && business->kind == BusinessNodeKind::RealEstateOffice;
}

bool isBankBusinessIndex(int32_t businessIndex) {
    const BusinessNodeDefinition* business = getBusinessNodeDefinition(businessIndex);
    return business != nullptr && business->kind == BusinessNodeKind::Bank;
}

bool isCarDealershipBusinessIndex(int32_t businessIndex) {
    const BusinessNodeDefinition* business = getBusinessNodeDefinition(businessIndex);
    return business != nullptr
        && (business->kind == BusinessNodeKind::CarDealershipNew
            || business->kind == BusinessNodeKind::CarDealershipUsed);
}

} // namespace Core
