#include "ui/map_status_hud.h"
#include "game/player_wallet.h"
#include "imgui.h"
#include <cstdio>
#include <cstring>

namespace Core {

namespace {

void renderHudChip(const char* title, const char* compactLine, const char* expandedBlock, ImVec2 anchorPos) {
    ImGui::SetNextWindowPos(anchorPos, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.82f);
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_AlwaysAutoResize
        | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNav;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 8.0f));
    ImGui::Begin(title, nullptr, flags);
    const bool isHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
    if (isHovered && expandedBlock != nullptr && expandedBlock[0] != '\0') {
        ImGui::Text("%s", title);
        ImGui::Separator();
        ImGui::TextWrapped("%s", expandedBlock);
    } else {
        ImGui::Text("%s", compactLine);
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

} // namespace

void renderMapStatusHud(
    const PlayerWallet& playerWallet,
    const PlayerLawEnforcementStore& lawStore,
    const PlayerLawIntelStore& intelStore,
    const PlayerHealthStore& healthStore,
    const GameCalendarStore& calendarStore,
    const ViewportPickState& viewportPickState,
    float canvasPosX,
    float canvasPosY,
    float canvasWidth,
    float canvasHeight) {
    (void)viewportPickState;
    char moneyCompact[48];
    char moneyExpanded[256];
    formatCashCents(moneyCompact, sizeof(moneyCompact), playerWallet.cashCents);
    std::snprintf(
        moneyExpanded,
        sizeof(moneyExpanded),
        "Cash: %s\nLegit income: %.2f c/tick\nIllegitimate income: %.2f c/tick\nHealth: %s",
        moneyCompact,
        playerWallet.legitIncomePerTickCents,
        playerWallet.crimeIncomePerTickCents,
        playerHealthStatusLabel(healthStore));
    char heatCompact[48];
    char heatExpanded[256];
    std::snprintf(heatCompact, sizeof(heatCompact), "Heat %d/%d", lawStore.personalHeat, PLAYER_HEAT_MAX);
    heatExpanded[0] = '\0';
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
    const float chipWidth = 170.0f;
    const ImVec2 moneyPos(canvasPosX + canvasWidth - chipWidth - 12.0f, canvasPosY + 12.0f);
    const ImVec2 heatPos(canvasPosX + canvasWidth - chipWidth - 12.0f, canvasPosY + 58.0f);
    const ImVec2 datePos(canvasPosX + canvasWidth - chipWidth - 12.0f, canvasPosY + 104.0f);
    renderHudChip("Money", moneyCompact, moneyExpanded, moneyPos);
    renderHudChip("Heat", heatCompact, heatExpanded, heatPos);
    renderHudChip("Date", dateCompact, dateCompact, datePos);
}

} // namespace Core
