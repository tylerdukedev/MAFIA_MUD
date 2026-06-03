#pragma once

#include "game/player_narrative_archive.h"
#include <cstdint>

namespace Core {

constexpr const char* PLAYTHROUGH_ARCHIVE_FILENAME = "capitalvice_playthrough_archive.dat";
constexpr int32_t MAX_PLAYTHROUGH_ARCHIVE_SLOTS = 8;
constexpr int32_t PLAYTHROUGH_ARCHIVE_NAME_LENGTH = 32;

struct PlaythroughArchiveSlot {
    char characterName[PLAYTHROUGH_ARCHIVE_NAME_LENGTH]{};
    uint64_t worldSeed = 0;
    int32_t totalDaysElapsed = 0;
    int64_t finalCashCents = 0;
    int32_t narrativeBeatCount = 0;
    NarrativeBeatRecord narrativeBeats[MAX_NARRATIVE_BEAT_COUNT]{};
    uint8_t isOccupied = 0;
};

struct PlaythroughArchiveFile {
    int32_t slotCount = MAX_PLAYTHROUGH_ARCHIVE_SLOTS;
    PlaythroughArchiveSlot slots[MAX_PLAYTHROUGH_ARCHIVE_SLOTS]{};
};

bool loadPlaythroughArchiveFile(PlaythroughArchiveFile& outArchive);
bool savePlaythroughArchiveFile(const PlaythroughArchiveFile& archive);
void upsertCurrentPlaythroughSlot(
    PlaythroughArchiveFile& archive,
    const char* characterName,
    uint64_t worldSeed,
    int32_t totalDaysElapsed,
    int64_t cashCents,
    const PlayerNarrativeArchiveStore& narrativeStore);

} // namespace Core
