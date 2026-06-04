#include "ui/map_status_hud.h"
#include "game/player_wallet.h"
#include "game/player_world_state.h"
#include "world/chunk_store.h"
#include "world/landmark_table.h"
#include "world/region_table.h"
#include "imgui.h"
#include <cstdio>
#include <cstring>

namespace Core {

namespace {

constexpr float MAP_HUD_MIN_CANVAS_WIDTH = 280.0f;
constexpr float MAP_HUD_CHIP_HEIGHT = 26.0f;
constexpr float MAP_HUD_PADDING = 8.0f;
constexpr float MAP_HUD_CHIP_GAP = 6.0f;

struct HudChipSpec {
    const char* id;
    const char* compactLine;
    const char* expandedBlock;
};

struct HudChipLayout {
    ImVec2 chipMin;
    ImVec2 chipMax;
    const HudChipSpec* spec;
    bool isHovered;
};

constexpr int32_t MAP_HUD_MAX_CHIP_COUNT = 8;

ImVec2 measureExpandedBlockSize(const char* expandedBlock) {
    if (expandedBlock == nullptr || expandedBlock[0] == '\0') {
        return ImVec2(0.0f, 0.0f);
    }
    return ImGui::CalcTextSize(expandedBlock, nullptr, false, 0.0f);
}

void drawExpandedBlock(ImDrawList* drawList, const ImVec2& textPos, const char* expandedBlock, ImU32 color) {
    if (expandedBlock == nullptr || expandedBlock[0] == '\0') {
        return;
    }
    const float lineHeight = ImGui::GetTextLineHeight();
    const char* lineStart = expandedBlock;
    float cursorY = textPos.y;
    while (lineStart[0] != '\0') {
        const char* lineEnd = lineStart;
        while (lineEnd[0] != '\0' && lineEnd[0] != '\n') {
            ++lineEnd;
        }
        drawList->AddText(ImVec2(textPos.x, cursorY), color, lineStart, lineEnd);
        cursorY += lineHeight;
        if (lineEnd[0] == '\n') {
            lineStart = lineEnd + 1;
        } else {
            break;
        }
    }
}

void formatPlayerLocationLabel(
    const ChunkStore& chunkStore,
    const PlayerWorldState& worldState,
    char* outBuffer,
    int32_t bufferSize) {
    if (outBuffer == nullptr || bufferSize <= 0) {
        return;
    }
    const WorldCoord coord{worldState.currentTileX, worldState.currentTileY};
    const RegionId regionId = chunkStore.getRegionAt(coord);
    const std::string_view regionName = RegionTable::getRegionShortName(regionId);
    int32_t nearestLandmarkIndex = -1;
    int32_t nearestDistanceSquared = 999999;
    const int32_t landmarkCount = getLandmarkCount();
    for (int32_t landmarkIndex = 0; landmarkIndex < landmarkCount; ++landmarkIndex) {
        const LandmarkDefinition* landmark = getLandmarkDefinition(landmarkIndex);
        if (landmark == nullptr) {
            continue;
        }
        const int32_t deltaX = landmark->tileX - worldState.currentTileX;
        const int32_t deltaY = landmark->tileY - worldState.currentTileY;
        const int32_t distanceSquared = deltaX * deltaX + deltaY * deltaY;
        if (distanceSquared < nearestDistanceSquared) {
            nearestDistanceSquared = distanceSquared;
            nearestLandmarkIndex = landmarkIndex;
        }
    }
    if (nearestLandmarkIndex >= 0 && nearestDistanceSquared <= 36) {
        const LandmarkDefinition* landmark = getLandmarkDefinition(nearestLandmarkIndex);
        std::snprintf(
            outBuffer,
            static_cast<size_t>(bufferSize),
            "%.*s · %s",
            static_cast<int>(regionName.size()),
            regionName.data(),
            landmark != nullptr ? landmark->mapLabel : "block");
        return;
    }
    std::snprintf(
        outBuffer,
        static_cast<size_t>(bufferSize),
        "%.*s (%d,%d)",
        static_cast<int>(regionName.size()),
        regionName.data(),
        worldState.currentTileX,
        worldState.currentTileY);
}

void renderHudChipBar(
    const HudChipSpec* chips,
    int32_t chipCount,
    float canvasPosX,
    float canvasPosY,
    float canvasWidth,
    MapHudInteraction& interaction) {
    if (chipCount <= 0 || canvasWidth < MAP_HUD_MIN_CANVAS_WIDTH) {
        return;
    }
    if (chipCount > MAP_HUD_MAX_CHIP_COUNT) {
        chipCount = MAP_HUD_MAX_CHIP_COUNT;
    }
    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    const float barHeight = MAP_HUD_CHIP_HEIGHT + MAP_HUD_PADDING * 2.0f;
    const ImVec2 barMin(canvasPosX, canvasPosY);
    const ImVec2 barMax(canvasPosX + canvasWidth, canvasPosY + barHeight);
    const ImVec2 mousePos = ImGui::GetMousePos();
    HudChipLayout layouts[MAP_HUD_MAX_CHIP_COUNT]{};
    int32_t visibleChipCount = 0;
    float cursorX = canvasPosX + MAP_HUD_PADDING;
    const float barY = canvasPosY + MAP_HUD_PADDING;
    for (int32_t chipIndex = 0; chipIndex < chipCount; ++chipIndex) {
        const HudChipSpec& chip = chips[chipIndex];
        const ImVec2 textSize = ImGui::CalcTextSize(chip.compactLine);
        const float chipWidth = textSize.x + 16.0f;
        if (cursorX + chipWidth > canvasPosX + canvasWidth - MAP_HUD_PADDING) {
            break;
        }
        const ImVec2 chipMin(cursorX, barY);
        const ImVec2 chipMax(cursorX + chipWidth, barY + MAP_HUD_CHIP_HEIGHT);
        const bool isHovered = mousePos.x >= chipMin.x && mousePos.x <= chipMax.x
            && mousePos.y >= chipMin.y && mousePos.y <= chipMax.y;
        layouts[visibleChipCount].chipMin = chipMin;
        layouts[visibleChipCount].chipMax = chipMax;
        layouts[visibleChipCount].spec = &chip;
        layouts[visibleChipCount].isHovered = isHovered;
        ++visibleChipCount;
        cursorX += chipWidth + MAP_HUD_CHIP_GAP;
    }
    drawList->PushClipRect(barMin, barMax, true);
    drawList->AddRectFilled(barMin, barMax, IM_COL32(10, 12, 16, 200));
    for (int32_t layoutIndex = 0; layoutIndex < visibleChipCount; ++layoutIndex) {
        const HudChipLayout& layout = layouts[layoutIndex];
        const HudChipSpec& chip = *layout.spec;
        const ImU32 fillColor = layout.isHovered ? IM_COL32(42, 48, 58, 240) : IM_COL32(28, 32, 40, 230);
        drawList->AddRectFilled(layout.chipMin, layout.chipMax, fillColor, 4.0f);
        drawList->AddRect(layout.chipMin, layout.chipMax, IM_COL32(70, 78, 92, 255), 4.0f);
        drawList->AddText(
            ImVec2(layout.chipMin.x + 8.0f, layout.chipMin.y + 5.0f),
            IM_COL32(230, 234, 242, 255),
            chip.compactLine);
    }
    drawList->PopClipRect();
    for (int32_t layoutIndex = 0; layoutIndex < visibleChipCount; ++layoutIndex) {
        const HudChipLayout& layout = layouts[layoutIndex];
        const HudChipSpec& chip = *layout.spec;
        if (!layout.isHovered || chip.expandedBlock == nullptr || chip.expandedBlock[0] == '\0') {
            continue;
        }
        if (chip.expandedBlock == chip.compactLine) {
            continue;
        }
        const ImVec2 tipSize = measureExpandedBlockSize(chip.expandedBlock);
        const ImVec2 tipMin(layout.chipMin.x, layout.chipMax.y + 4.0f);
        const ImVec2 tipMax(tipMin.x + tipSize.x + 12.0f, tipMin.y + tipSize.y + 10.0f);
        drawList->AddRectFilled(tipMin, tipMax, IM_COL32(20, 24, 30, 245), 3.0f);
        drawList->AddRect(tipMin, tipMax, IM_COL32(70, 78, 92, 255), 3.0f);
        drawExpandedBlock(drawList, ImVec2(tipMin.x + 6.0f, tipMin.y + 4.0f), chip.expandedBlock, IM_COL32(220, 226, 235, 255));
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            if (std::strcmp(chip.id, "location") == 0) {
                interaction.requestCenterOnPlayer = true;
                interaction.requestFocusCharacterPanel = true;
            }
        }
    }
}

} // namespace

void renderMapStatusHud(
    const PlayerWallet& playerWallet,
    const PlayerLawEnforcementStore& lawStore,
    const PlayerLawIntelStore& intelStore,
    const PlayerHealthStore& healthStore,
    const GameCalendarStore& calendarStore,
    const ChunkStore& chunkStore,
    const PlayerWorldState& worldState,
    MapHudInteraction& interaction,
    float canvasPosX,
    float canvasPosY,
    float canvasWidth,
    float canvasHeight) {
    (void)canvasHeight;
    char moneyCompact[48];
    char moneyExpanded[256];
    formatCashCents(moneyCompact, sizeof(moneyCompact), playerWallet.cashCents);
    std::snprintf(
        moneyExpanded,
        sizeof(moneyExpanded),
        "Cash: %s\nLegit: %.2f c/tick\nIllegitimate: %.2f c/tick\nHealth: %s",
        moneyCompact,
        playerWallet.legitIncomePerTickCents,
        playerWallet.crimeIncomePerTickCents,
        playerHealthStatusLabel(healthStore));
    char heatCompact[48];
    char heatExpanded[256];
    std::snprintf(heatCompact, sizeof(heatCompact), "Heat %d/%d", lawStore.personalHeat, PLAYER_HEAT_MAX);
    std::snprintf(
        heatExpanded,
        sizeof(heatExpanded),
        "Status: %s\nPersonal heat: %d / %d",
        getPoliceInvestigationLabel(lawStore.investigationTier),
        lawStore.personalHeat,
        PLAYER_HEAT_MAX);
    if (doesPlayerKnowAboutWarrants(intelStore, lawStore)) {
        char warrantLine[64];
        std::snprintf(
            warrantLine,
            sizeof(warrantLine),
            "\nKnown warrants: %d",
            getPlayerKnownWarrantCount(intelStore, lawStore));
        std::strncat(heatExpanded, warrantLine, sizeof(heatExpanded) - std::strlen(heatExpanded) - 1);
    }
    char dateCompact[64];
    formatCalendarDateLabel(calendarStore, dateCompact, sizeof(dateCompact));
    char locationCompact[80];
    formatPlayerLocationLabel(chunkStore, worldState, locationCompact, sizeof(locationCompact));
    char locationExpanded[128];
    std::snprintf(locationExpanded, sizeof(locationExpanded), "Current position\n%s", locationCompact);
    const HudChipSpec chips[] = {
        {"money", moneyCompact, moneyExpanded},
        {"heat", heatCompact, heatExpanded},
        {"location", locationCompact, locationExpanded},
        {"date", dateCompact, dateCompact},
    };
    renderHudChipBar(chips, 4, canvasPosX, canvasPosY, canvasWidth, interaction);
}

void renderMapTileCoordReadout(
    const ViewportPickState& viewportPickState,
    float canvasPosX,
    float canvasPosY,
    float canvasWidth,
    float canvasHeight) {
    if (canvasWidth < MAP_HUD_MIN_CANVAS_WIDTH || canvasHeight < MAP_HUD_CHIP_HEIGHT + MAP_HUD_PADDING * 4.0f) {
        return;
    }
    char coordLine[40];
    if (!viewportPickState.hasHover) {
        std::snprintf(coordLine, sizeof(coordLine), "Tile (--, --)");
    } else {
        std::snprintf(
            coordLine,
            sizeof(coordLine),
            "Tile (%d, %d)",
            viewportPickState.hoveredCoord.x,
            viewportPickState.hoveredCoord.y);
    }
    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    const ImVec2 textSize = ImGui::CalcTextSize(coordLine);
    constexpr float MAP_COORD_PADDING = 8.0f;
    const ImVec2 textPos(
        canvasPosX + MAP_COORD_PADDING,
        canvasPosY + canvasHeight - textSize.y - MAP_COORD_PADDING);
    const ImVec2 bgMin(textPos.x - 4.0f, textPos.y - 2.0f);
    const ImVec2 bgMax(textPos.x + textSize.x + 4.0f, textPos.y + textSize.y + 2.0f);
    drawList->AddRectFilled(bgMin, bgMax, IM_COL32(10, 12, 16, 200), 3.0f);
    drawList->AddText(textPos, IM_COL32(220, 224, 232, 255), coordLine);
}

} // namespace Core
