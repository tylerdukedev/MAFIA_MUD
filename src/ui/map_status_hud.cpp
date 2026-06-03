#include "ui/map_status_hud.h"
#include "game/player_wallet.h"
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
    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    const ImVec2 clipMin(canvasPosX, canvasPosY);
    const ImVec2 clipMax(canvasPosX + canvasWidth, canvasPosY + MAP_HUD_CHIP_HEIGHT + MAP_HUD_PADDING * 2.0f);
    drawList->PushClipRect(clipMin, clipMax, true);
    drawList->AddRectFilled(
        ImVec2(canvasPosX, canvasPosY),
        ImVec2(canvasPosX + canvasWidth, canvasPosY + MAP_HUD_CHIP_HEIGHT + MAP_HUD_PADDING * 2.0f),
        IM_COL32(10, 12, 16, 200));
    float cursorX = canvasPosX + MAP_HUD_PADDING;
    const float barY = canvasPosY + MAP_HUD_PADDING;
    const ImVec2 mousePos = ImGui::GetMousePos();
    for (int32_t chipIndex = 0; chipIndex < chipCount; ++chipIndex) {
        const HudChipSpec& chip = chips[chipIndex];
        const ImVec2 textSize = ImGui::CalcTextSize(chip.compactLine);
        const float chipWidth = textSize.x + 16.0f;
        const ImVec2 chipMin(cursorX, barY);
        const ImVec2 chipMax(cursorX + chipWidth, barY + MAP_HUD_CHIP_HEIGHT);
        const bool isHovered = mousePos.x >= chipMin.x && mousePos.x <= chipMax.x && mousePos.y >= chipMin.y && mousePos.y <= chipMax.y;
        const ImU32 fillColor = isHovered ? IM_COL32(42, 48, 58, 240) : IM_COL32(28, 32, 40, 230);
        drawList->AddRectFilled(chipMin, chipMax, fillColor, 4.0f);
        drawList->AddRect(chipMin, chipMax, IM_COL32(70, 78, 92, 255), 4.0f);
        drawList->AddText(ImVec2(chipMin.x + 8.0f, chipMin.y + 5.0f), IM_COL32(230, 234, 242, 255), chip.compactLine);
        if (isHovered && chip.expandedBlock != nullptr && chip.expandedBlock[0] != '\0') {
            const ImVec2 tipSize = ImGui::CalcTextSize(chip.expandedBlock);
            const ImVec2 tipMin(chipMin.x, chipMax.y + 4.0f);
            const ImVec2 tipMax(tipMin.x + tipSize.x + 12.0f, tipMin.y + tipSize.y + 10.0f);
            drawList->AddRectFilled(tipMin, tipMax, IM_COL32(20, 24, 30, 245), 3.0f);
            drawList->AddText(ImVec2(tipMin.x + 6.0f, tipMin.y + 4.0f), IM_COL32(220, 226, 235, 255), chip.expandedBlock);
        }
        if (isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            if (std::strcmp(chip.id, "location") == 0) {
                interaction.requestCenterOnPlayer = true;
                interaction.requestFocusCharacterPanel = true;
            }
        }
        cursorX += chipWidth + MAP_HUD_CHIP_GAP;
        if (cursorX > canvasPosX + canvasWidth - MAP_HUD_PADDING) {
            break;
        }
    }
    drawList->PopClipRect();
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

} // namespace Core
