#include "ui/map_marker_style.h"

namespace Core {

namespace {

ImU32 brightenMarkerColor(ImU32 color, int32_t deltaR, int32_t deltaG, int32_t deltaB, int32_t deltaA) {
    const int32_t r = static_cast<int32_t>((color >> IM_COL32_R_SHIFT) & 0xFF) + deltaR;
    const int32_t g = static_cast<int32_t>((color >> IM_COL32_G_SHIFT) & 0xFF) + deltaG;
    const int32_t b = static_cast<int32_t>((color >> IM_COL32_B_SHIFT) & 0xFF) + deltaB;
    const int32_t a = static_cast<int32_t>((color >> IM_COL32_A_SHIFT) & 0xFF) + deltaA;
    const auto clampChannel = [](int32_t value) -> uint8_t {
        if (value < 0) {
            return 0;
        }
        if (value > 255) {
            return 255;
        }
        return static_cast<uint8_t>(value);
    };
    return IM_COL32(clampChannel(r), clampChannel(g), clampChannel(b), clampChannel(a));
}

ImU32 getBaseLocationMarkerColor(LocationCategory category) {
    switch (category) {
    case LocationCategory::Landmark:
        return IM_COL32(240, 244, 252, 230);
    case LocationCategory::IndustryRetail:
        return IM_COL32(70, 130, 255, 240);
    case LocationCategory::IndustryFoodService:
        return IM_COL32(255, 150, 70, 240);
    case LocationCategory::IndustryLogistics:
        return IM_COL32(90, 170, 210, 240);
    case LocationCategory::IndustryManufacturing:
        return IM_COL32(180, 120, 70, 240);
    case LocationCategory::IndustryOffice:
        return IM_COL32(130, 150, 220, 240);
    case LocationCategory::IndustryHospitality:
        return IM_COL32(220, 110, 170, 240);
    case LocationCategory::IndustryConstruction:
        return IM_COL32(200, 170, 60, 240);
    case LocationCategory::Law:
        return IM_COL32(170, 120, 230, 240);
    case LocationCategory::RealEstate:
        return IM_COL32(60, 190, 140, 240);
    case LocationCategory::Bank:
        return IM_COL32(230, 200, 70, 240);
    case LocationCategory::CarDealershipNew:
        return IM_COL32(80, 180, 255, 240);
    case LocationCategory::CarDealershipUsed:
        return IM_COL32(210, 120, 70, 240);
    case LocationCategory::PropertyListing:
        return IM_COL32(230, 100, 150, 240);
    default:
        return IM_COL32(200, 200, 200, 230);
    }
}

} // namespace

ImU32 getLocationMarkerColor(LocationCategory category, bool isHovered, bool isSelected) {
    ImU32 color = getBaseLocationMarkerColor(category);
    if (isSelected) {
        color = brightenMarkerColor(color, 20, 20, 10, 25);
    } else if (isHovered) {
        color = brightenMarkerColor(color, 15, 15, 10, 15);
    }
    return color;
}

LocationCategory businessIndustryToLocationCategory(BusinessIndustry industry) {
    switch (industry) {
    case BusinessIndustry::Retail:
        return LocationCategory::IndustryRetail;
    case BusinessIndustry::FoodService:
        return LocationCategory::IndustryFoodService;
    case BusinessIndustry::Logistics:
        return LocationCategory::IndustryLogistics;
    case BusinessIndustry::Manufacturing:
        return LocationCategory::IndustryManufacturing;
    case BusinessIndustry::Office:
        return LocationCategory::IndustryOffice;
    case BusinessIndustry::Hospitality:
        return LocationCategory::IndustryHospitality;
    case BusinessIndustry::Construction:
        return LocationCategory::IndustryConstruction;
    default:
        return LocationCategory::IndustryOffice;
    }
}

LocationCategory getBusinessLocationCategory(const BusinessNodeDefinition& business) {
    switch (business.kind) {
    case BusinessNodeKind::LawOffice:
        return LocationCategory::Law;
    case BusinessNodeKind::RealEstateOffice:
        return LocationCategory::RealEstate;
    case BusinessNodeKind::Bank:
        return LocationCategory::Bank;
    case BusinessNodeKind::CarDealershipNew:
        return LocationCategory::CarDealershipNew;
    case BusinessNodeKind::CarDealershipUsed:
        return LocationCategory::CarDealershipUsed;
    default:
        return businessIndustryToLocationCategory(business.industry);
    }
}

LocationCategory getBusinessLocationCategory(int32_t businessIndex) {
    const BusinessNodeDefinition* business = getBusinessNodeDefinition(businessIndex);
    if (business == nullptr) {
        return LocationCategory::IndustryOffice;
    }
    return getBusinessLocationCategory(*business);
}

void drawMapMarkerCircle(
    ImDrawList* drawList,
    float centerX,
    float centerY,
    float radiusPixels,
    ImU32 fillColor,
    float outlineThickness) {
    drawList->AddCircleFilled(ImVec2(centerX, centerY), radiusPixels, fillColor);
    drawList->AddCircle(
        ImVec2(centerX, centerY),
        radiusPixels + 1.0f,
        MAP_MARKER_OUTLINE_COLOR,
        0,
        outlineThickness);
}

void getMapMarkerLabelBounds(
    float centerX,
    float centerY,
    float markerRadiusPixels,
    const char* labelText,
    ImVec2& outMin,
    ImVec2& outMax) {
    const ImVec2 labelSize = ImGui::CalcTextSize(labelText);
    const ImVec2 labelPos(centerX - labelSize.x * 0.5f, centerY + markerRadiusPixels + MAP_MARKER_LABEL_OFFSET_Y);
    outMin = ImVec2(labelPos.x - MAP_MARKER_LABEL_PADDING_X, labelPos.y - MAP_MARKER_LABEL_PADDING_Y);
    outMax = ImVec2(labelPos.x + labelSize.x + MAP_MARKER_LABEL_PADDING_X, labelPos.y + labelSize.y + MAP_MARKER_LABEL_PADDING_Y);
}

void drawMapMarkerLabel(
    ImDrawList* drawList,
    float centerX,
    float centerY,
    float markerRadiusPixels,
    const char* labelText,
    ImU32 textColor) {
    ImVec2 labelMin{};
    ImVec2 labelMax{};
    getMapMarkerLabelBounds(centerX, centerY, markerRadiusPixels, labelText, labelMin, labelMax);
    const ImVec2 labelPos(labelMin.x + MAP_MARKER_LABEL_PADDING_X, labelMin.y + MAP_MARKER_LABEL_PADDING_Y);
    drawList->AddRectFilled(labelMin, labelMax, MAP_MARKER_LABEL_BG_COLOR);
    drawList->AddText(labelPos, textColor, labelText);
}

} // namespace Core
