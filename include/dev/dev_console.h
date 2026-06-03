#pragma once

#include "character/character_draft.h"
#include "character/player_profile.h"
#include "game/player_wallet.h"
#include "world/city_control.h"
#include <cstddef>
#include <cstdint>

struct ImGuiContext;

namespace Core {

struct DevConsoleState {
    bool isVisible = false;
    char inputBuffer[256]{};
    bool requestFocusInput = false;
};

constexpr int32_t DEV_CONSOLE_LOG_LINE_COUNT = 128;
constexpr int32_t DEV_CONSOLE_LOG_LINE_SIZE = 128;

struct DevConsoleLog {
    char lines[DEV_CONSOLE_LOG_LINE_COUNT][DEV_CONSOLE_LOG_LINE_SIZE]{};
    int32_t writeIndex = 0;
    int32_t lineCount = 0;
};

struct DevConsoleGameplaySnapshot {
    const PlayerWallet* playerWallet = nullptr;
    const CityControlStore* cityControlStore = nullptr;
    bool isWorldReady = false;
};

void devConsoleLogAppend(DevConsoleLog& log, const char* message);
void devConsoleLogClear(DevConsoleLog& log);
void devConsoleToggleVisibility(DevConsoleState& state);
void devConsoleExecuteCommand(
    DevConsoleLog& log,
    const char* commandLine,
    CharacterDraft& draft,
    PlayerProfile& profile,
    const DevConsoleGameplaySnapshot* gameplaySnapshot = nullptr);
void devConsoleRender(
    DevConsoleState& state,
    DevConsoleLog& log,
    CharacterDraft& draft,
    PlayerProfile& profile,
    const DevConsoleGameplaySnapshot* gameplaySnapshot = nullptr);
bool devConsoleParseProfileSetCommand(
    const char* commandLine,
    CharacterDraft& draft,
    PlayerProfile& profile,
    char* outMessage,
    size_t messageSize);

} // namespace Core
