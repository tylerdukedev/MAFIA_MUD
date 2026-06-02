#pragma once

#include "character/character_draft.h"
#include "core/sim_clock.h"
#include "sim/system_registry.h"
#include "ui/map_camera.h"
#include "ui/viewport_state.h"
#include "world/chunk_store.h"
#include "world/world_config.h"
#include <cstdint>

namespace Core {

enum class FrontendScreen : uint8_t {
    MainMenu = 0,
    CharacterCreation,
    InGame
};

struct FrontendUiEvents {
    bool requestedNewGame = false;
    bool requestedLoadGame = false;
    bool requestedExitGame = false;
    bool requestedStartSimulation = false;
};

struct GameUiEvents {
    bool requestedSaveGame = false;
    bool requestedLoadGame = false;
};

void setSaveLoadStatusMessage(const char* message);
const char* getSaveLoadStatusMessage();

FrontendUiEvents renderFrontendUi(FrontendScreen& frontendScreen, CharacterDraft& characterDraft, bool hasSaveFile);

GameUiEvents renderGameUi(
    SimClock& simClock,
    const WorldConfig& worldConfig,
    const ChunkStore& chunkStore,
    SystemRegistry& systemRegistry,
    MapCamera& mapCamera,
    ViewportPickState& viewportPickState,
    uint64_t worldSeed,
    bool hasSaveFile);

} // namespace Core
