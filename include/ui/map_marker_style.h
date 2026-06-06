#pragma once

#include "world/business_node_table.h"
#include "imgui.h"
#include <cstdint>

namespace Core {

enum class LocationCategory : uint8_t {
    Landmark = 0,
    IndustryRetail,
    IndustryFoodService,
    IndustryLogistics,
    IndustryManufacturing,
    IndustryOffice,
    IndustryHospitality,
    IndustryConstruction,
    Law,
    RealEstate,
    Bank,
    CarDealershipNew,
    CarDealershipUsed,
    PropertyListing,
};

constexpr ImU32 MAP_MARKER_OUTLINE_COLOR = IM_COL32(12, 14, 18, 255);
constexpr float MAP_LABEL_ZOOM_THRESHOLD = 4.5f;
constexpr float MAP_MARKER_DEFAULT_RADIUS_PIXELS = 4.0f;
constexpr float MAP_MARKER_LABEL_PADDING_X = 3.0f;
constexpr float MAP_MARKER_LABEL_PADDING_Y = 1.0f;
constexpr float MAP_MARKER_LABEL_OFFSET_Y = 2.0f;
constexpr ImU32 MAP_MARKER_LABEL_BG_COLOR = IM_COL32(16, 18, 24, 210);
constexpr ImU32 MAP_MARKER_LABEL_TEXT_COLOR = IM_COL32(235, 238, 245, 245);

ImU32 getLocationMarkerColor(LocationCategory category, bool isHovered, bool isSelected);
LocationCategory businessIndustryToLocationCategory(BusinessIndustry industry);
LocationCategory getBusinessLocationCategory(const BusinessNodeDefinition& business);
LocationCategory getBusinessLocationCategory(int32_t businessIndex);

void drawMapMarkerCircle(
    ImDrawList* drawList,
    float centerX,
    float centerY,
    float radiusPixels,
    ImU32 fillColor,
    float outlineThickness = 1.5f);

void drawMapMarkerLabel(
    ImDrawList* drawList,
    float centerX,
    float centerY,
    float markerRadiusPixels,
    const char* labelText,
    ImU32 textColor = MAP_MARKER_LABEL_TEXT_COLOR);

void getMapMarkerLabelBounds(
    float centerX,
    float centerY,
    float markerRadiusPixels,
    const char* labelText,
    ImVec2& outMin,
    ImVec2& outMax);

} // namespace Core
