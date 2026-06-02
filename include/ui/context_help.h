#pragma once

#include <cstdint>

namespace Core {

struct HelpManualState;

struct ContextHelpState {
    bool isInspectMode = false;
    bool showInspectPopup = false;
    bool hasHoveredTarget = false;
    char hoveredTitle[128]{};
    char hoveredTooltip[256]{};
    char hoveredManualTopicId[64]{};
    char popupTitle[128]{};
    char popupTooltip[256]{};
    char popupManualTopicId[64]{};
};

void updateContextHelpMode(ContextHelpState& state);
void renderContextHelpCursorOverlay(const ContextHelpState& state);
void renderContextHelpInspectPopup(ContextHelpState& state, HelpManualState& manualState);
void contextHelpTextLine(const char* text, const char* tooltip, const char* manualTopicId, ContextHelpState& state);
void contextHelpStatBar(const char* label, float valueNormalized, const char* tooltip, const char* manualTopicId, ContextHelpState& state);
void contextHelpSectionHeader(const char* title, const char* tooltip, const char* manualTopicId, ContextHelpState& state);
void contextHelpPanelTag(const char* panelTitle, const char* tooltip, const char* manualTopicId, ContextHelpState& state);
void contextHelpWrappedText(const char* text, const char* title, const char* tooltip, const char* manualTopicId, ContextHelpState& state);

} // namespace Core
