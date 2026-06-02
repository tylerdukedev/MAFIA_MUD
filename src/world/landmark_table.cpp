#include "world/landmark_table.h"

namespace Core {

namespace {
constexpr LandmarkRect LANDMARKS[] = {
    {LandmarkId::CentralPark, RegionId::Manhattan, TerrainId::Park, true, 270, 116, 287, 168},
    {LandmarkId::LaGuardiaAirport, RegionId::Queens, TerrainId::Airport, false, 402, 150, 438, 170},
    {LandmarkId::JfkAirport, RegionId::Queens, TerrainId::Airport, false, 416, 372, 458, 398},
};
constexpr int32_t LANDMARK_COUNT = 3;
} // namespace

std::string_view LandmarkTable::getLandmarkName(LandmarkId landmarkId) {
    switch (landmarkId) {
    case LandmarkId::None: return "None";
    case LandmarkId::CentralPark: return "Central Park";
    case LandmarkId::LaGuardiaAirport: return "LaGuardia Airport";
    case LandmarkId::JfkAirport: return "JFK Airport";
    default: return "Unknown";
    }
}

int32_t LandmarkTable::getLandmarkCount() {
    return LANDMARK_COUNT;
}

const LandmarkRect& LandmarkTable::getLandmark(int32_t index) {
    return LANDMARKS[index];
}

} // namespace Core
