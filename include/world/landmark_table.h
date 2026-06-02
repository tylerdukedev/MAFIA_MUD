#pragma once

#include "core/types.h"
#include <cstdint>

namespace Core {

constexpr int32_t MAX_LANDMARK_COUNT = 64;
constexpr float LANDMARK_BASE_HEAT = 0.85f;
constexpr float LANDMARK_CONTROL_DIFFICULTY = 0.95f;
constexpr float LANDMARK_BOROUGH_INFLUENCE_WEIGHT = 1.5f;

struct LandmarkDefinition {
    const char* id;
    int32_t tileX;
    int32_t tileY;
    const char* fullName;
    const char* mapLabel;
};

int32_t getLandmarkCount();
const LandmarkDefinition* getLandmarkDefinition(int32_t landmarkIndex);
int32_t findLandmarkIndexAtTile(int32_t tileX, int32_t tileY);
const char* getLandmarkTooltipText(int32_t landmarkIndex);

} // namespace Core
