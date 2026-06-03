#include "world/landmark_table.h"
#include "utils/seed_hash.h"
#include <cstring>

namespace Core {

namespace {

constexpr LandmarkDefinition LANDMARK_DEFINITIONS[] = {
    {"financial_district", 229, 248, "Financial District", "Financial District", 1, 220},
    {"lower_manhattan", 235, 234, "Lower Manhattan", "Lower Manhattan", 1, 200},
    {"soho", 232, 229, "Soho", "Soho", 1, 200},
    {"midtown_manhattan", 243, 208, "Midtown Manhattan", "Midtown Manhattan", 1, 210},
    {"times_square", 234, 192, "Times Square", "Times Square", 2, 230},
    {"upper_east_side", 271, 181, "Upper East Side", "Upper East Side", 1, 200},
    {"upper_west_side", 253, 160, "Upper West Side", "Upper West Side", 1, 200},
    {"upper_manhattan", 281, 117, "Upper Manhattan", "Upper Manhattan", 1, 200},
    {"washington_heights", 287, 100, "Washington Heights", "Washington Heights", 1, 200},
    {"hunts_point", 343, 129, "Hunts Point", "Hunts Point", 1, 200},
    {"longwood", 337, 114, "Longwood", "Longwood", 1, 200},
    {"west_bronx", 328, 91, "West Bronx", "West Bronx", 1, 200},
    {"east_bronx", 371, 65, "East Bronx", "East Bronx", 1, 200},
    {"morris_park", 373, 86, "Morris Park", "Morris Park", 1, 200},
    {"parkchester", 368, 93, "Parkchester", "Parkchester", 1, 200},
    {"brighton_beach", 275, 406, "Brighton Beach", "Brighton Beach", 1, 200},
    {"sheepshead_bay", 291, 387, "Sheepshead Bay", "Sheepshead Bay", 1, 200},
    {"flatbush", 268, 350, "Flatbush", "Flatbush", 1, 200},
    {"canarsie", 312, 352, "Canarsie", "Canarsie", 1, 200},
    {"bensonhurst", 246, 367, "Bensonhurst", "Bensonhurst", 1, 200},
    {"bay_ridge", 212, 345, "Bay Ridge", "Bay Ridge", 1, 200},
    {"bushwick", 311, 299, "Bushwick", "Bushwick", 1, 200},
    {"williamsburg", 272, 265, "Williamsburg", "Williamsburg", 1, 210},
    {"laguardia_airport", 331, 170, "LaGuardia Airport", "LGA", 2, 210},
    {"jackson_heights", 361, 205, "Jackson Heights", "Jackson Heights", 1, 200},
    {"astoria", 313, 201, "Astoria", "Astoria", 1, 200},
    {"jamaica", 426, 258, "Jamaica", "Jamaica", 1, 200},
    {"whitestone", 413, 153, "Whitestone", "Whitestone", 1, 200},
    {"port_richmond", 96, 345, "Port Richmond", "Port Richmond", 1, 200},
    {"mid_island", 102, 382, "Mid Island", "Mid Island", 1, 200},
    {"midland_beach", 149, 414, "Midland Beach", "Midland Beach", 1, 200},
    {"great_kills", 89, 434, "Great Kills", "Great Kills", 1, 200},
    {"eltingville", 90, 457, "Eltingville", "Eltingville", 1, 200},
    {"hells_kitchen", 248, 215, "Hell's Kitchen", "Hell's Kitchen", 1, 210},
    {"dumbo", 262, 278, "DUMBO", "DUMBO", 1, 215},
    {"coney_island", 251, 411, "Coney Island", "Coney Island", 2, 205},
    {"yankee_stadium", 318, 108, "Yankee Stadium", "Yankee Stadium", 2, 220},
    {"newark_penn", 112, 174, "Newark Penn Station", "Newark Penn", 1, 210},
};

constexpr int32_t LANDMARK_COUNT = static_cast<int32_t>(sizeof(LANDMARK_DEFINITIONS) / sizeof(LANDMARK_DEFINITIONS[0]));

constexpr RegionId LANDMARK_REGION_IDS[] = {
    RegionId::Manhattan, RegionId::Manhattan, RegionId::Manhattan, RegionId::Manhattan, RegionId::Manhattan,
    RegionId::Manhattan, RegionId::Manhattan, RegionId::Manhattan, RegionId::Manhattan,
    RegionId::Bronx, RegionId::Bronx, RegionId::Bronx, RegionId::Bronx, RegionId::Bronx, RegionId::Bronx,
    RegionId::Brooklyn, RegionId::Brooklyn, RegionId::Brooklyn, RegionId::Brooklyn, RegionId::Brooklyn,
    RegionId::Brooklyn, RegionId::Brooklyn, RegionId::Brooklyn, RegionId::Brooklyn,
    RegionId::Queens, RegionId::Queens, RegionId::Queens, RegionId::Queens, RegionId::Queens,
    RegionId::StatenIsland, RegionId::StatenIsland, RegionId::StatenIsland, RegionId::StatenIsland, RegionId::StatenIsland,
    RegionId::Manhattan, RegionId::Brooklyn, RegionId::Bronx, RegionId::Brooklyn, RegionId::NewJersey,
};

} // namespace

int32_t getLandmarkCount() {
    return LANDMARK_COUNT;
}

const LandmarkDefinition* getLandmarkDefinition(int32_t landmarkIndex) {
    if (landmarkIndex < 0 || landmarkIndex >= LANDMARK_COUNT) {
        return nullptr;
    }
    return &LANDMARK_DEFINITIONS[landmarkIndex];
}

int32_t findLandmarkIndexAtTile(int32_t tileX, int32_t tileY) {
    for (int32_t landmarkIndex = 0; landmarkIndex < LANDMARK_COUNT; ++landmarkIndex) {
        const LandmarkDefinition& landmark = LANDMARK_DEFINITIONS[landmarkIndex];
        if (landmark.tileX == tileX && landmark.tileY == tileY) {
            return landmarkIndex;
        }
    }
    return -1;
}

const char* getLandmarkTooltipText(int32_t landmarkIndex) {
    const LandmarkDefinition* landmark = getLandmarkDefinition(landmarkIndex);
    if (landmark == nullptr) {
        return "";
    }
    return landmark->fullName;
}

RegionId getLandmarkRegionId(int32_t landmarkIndex) {
    if (landmarkIndex < 0 || landmarkIndex >= LANDMARK_COUNT) {
        return RegionId::None;
    }
    return LANDMARK_REGION_IDS[landmarkIndex];
}

RegionId regionIdFromBoroughPreferenceIndex(int32_t boroughIndex) {
    switch (boroughIndex) {
    case 0: return RegionId::Manhattan;
    case 1: return RegionId::Brooklyn;
    case 2: return RegionId::Queens;
    case 3: return RegionId::Bronx;
    case 4: return RegionId::StatenIsland;
    default: return RegionId::Manhattan;
    }
}

int32_t pickRandomLandmarkIndexInRegion(RegionId regionId, uint64_t seed, int32_t salt) {
    int32_t matches[MAX_LANDMARK_COUNT];
    int32_t matchCount = 0;
    for (int32_t landmarkIndex = 0; landmarkIndex < LANDMARK_COUNT; ++landmarkIndex) {
        if (LANDMARK_REGION_IDS[landmarkIndex] != regionId) {
            continue;
        }
        matches[matchCount] = landmarkIndex;
        ++matchCount;
    }
    if (matchCount <= 0) {
        return -1;
    }
    const uint32_t pick = Utils::hashSeedMix(seed, salt, static_cast<int32_t>(regionId)) % static_cast<uint32_t>(matchCount);
    return matches[pick];
}

} // namespace Core
