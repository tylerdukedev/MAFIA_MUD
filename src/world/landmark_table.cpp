#include "world/landmark_table.h"
#include <cstring>

namespace Core {

namespace {

constexpr LandmarkDefinition LANDMARK_DEFINITIONS[] = {
    {"financial_district", 229, 248, "Financial District", "Financial District"},
    {"lower_manhattan", 235, 234, "Lower Manhattan", "Lower Manhattan"},
    {"soho", 234, 232, "Soho", "Soho"},
    {"midtown_manhattan", 243, 208, "Midtown Manhattan", "Midtown Manhattan"},
    {"times_square", 234, 192, "Times Square", "Times Square"},
    {"upper_east_side", 271, 181, "Upper East Side", "Upper East Side"},
    {"upper_west_side", 253, 160, "Upper West Side", "Upper West Side"},
    {"upper_manhattan", 281, 117, "Upper Manhattan", "Upper Manhattan"},
    {"washington_heights", 287, 100, "Washington Heights", "Washington Heights"},
    {"hunts_point", 343, 129, "Hunts Point", "Hunts Point"},
    {"longwood", 337, 114, "Longwood", "Longwood"},
    {"west_bronx", 328, 91, "West Bronx", "West Bronx"},
    {"east_bronx", 371, 65, "East Bronx", "East Bronx"},
    {"morris_park", 373, 86, "Morris Park", "Morris Park"},
    {"parkchester", 368, 93, "Parkchester", "Parkchester"},
    {"brighton_beach", 275, 406, "Brighton Beach", "Brighton Beach"},
    {"sheepshead_bay", 291, 387, "Sheepshead Bay", "Sheepshead Bay"},
    {"flatbush", 268, 350, "Flatbush", "Flatbush"},
    {"canarsie", 312, 352, "Canarsie", "Canarsie"},
    {"bensonhurst", 246, 367, "Bensonhurst", "Bensonhurst"},
    {"bay_ridge", 212, 345, "Bay Ridge", "Bay Ridge"},
    {"bushwick", 311, 299, "Bushwick", "Bushwick"},
    {"williamsburg", 272, 265, "Williamsburg", "Williamsburg"},
    {"laguardia_airport", 331, 170, "LaGuardia Airport", "LGA"},
    {"jackson_heights", 361, 205, "Jackson Heights", "Jackson Heights"},
    {"astoria", 313, 201, "Astoria", "Astoria"},
    {"jamaica", 426, 258, "Jamaica", "Jamaica"},
    {"whitestone", 413, 153, "Whitestone", "Whitestone"},
    {"port_richmond", 96, 345, "Port Richmond", "Port Richmond"},
    {"mid_island", 102, 382, "Mid Island", "Mid Island"},
    {"midland_beach", 149, 414, "Midland Beach", "Midland Beach"},
    {"great_kills", 89, 434, "Great Kills", "Great Kills"},
    {"eltingville", 90, 457, "Eltingville", "Eltingville"},
};

constexpr int32_t LANDMARK_COUNT = static_cast<int32_t>(sizeof(LANDMARK_DEFINITIONS) / sizeof(LANDMARK_DEFINITIONS[0]));

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

} // namespace Core
