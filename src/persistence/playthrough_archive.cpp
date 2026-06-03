#include "persistence/playthrough_archive.h"
#include <cstdio>
#include <cstring>

namespace Core {

namespace {
constexpr uint32_t PLAYTHROUGH_ARCHIVE_VERSION = 1U;
} // namespace

bool loadPlaythroughArchiveFile(PlaythroughArchiveFile& outArchive) {
    FILE* fileHandle = std::fopen(PLAYTHROUGH_ARCHIVE_FILENAME, "rb");
    if (fileHandle == nullptr) {
        outArchive = PlaythroughArchiveFile{};
        return true;
    }
    uint32_t version = 0U;
    const bool versionRead = std::fread(&version, sizeof(version), 1, fileHandle) == 1;
    if (!versionRead || version != PLAYTHROUGH_ARCHIVE_VERSION) {
        std::fclose(fileHandle);
        outArchive = PlaythroughArchiveFile{};
        return false;
    }
    const bool slotsRead = std::fread(outArchive.slots, sizeof(outArchive.slots), 1, fileHandle) == 1;
    std::fclose(fileHandle);
    if (!slotsRead) {
        outArchive = PlaythroughArchiveFile{};
        return false;
    }
    outArchive.slotCount = MAX_PLAYTHROUGH_ARCHIVE_SLOTS;
    return true;
}

bool savePlaythroughArchiveFile(const PlaythroughArchiveFile& archive) {
    FILE* fileHandle = std::fopen(PLAYTHROUGH_ARCHIVE_FILENAME, "wb");
    if (fileHandle == nullptr) {
        return false;
    }
    const uint32_t version = PLAYTHROUGH_ARCHIVE_VERSION;
    const bool versionWritten = std::fwrite(&version, sizeof(version), 1, fileHandle) == 1;
    const bool slotsWritten = std::fwrite(archive.slots, sizeof(archive.slots), 1, fileHandle) == 1;
    std::fclose(fileHandle);
    return versionWritten && slotsWritten;
}

void upsertCurrentPlaythroughSlot(
    PlaythroughArchiveFile& archive,
    const char* characterName,
    uint64_t worldSeed,
    int32_t totalDaysElapsed,
    int64_t cashCents,
    const PlayerNarrativeArchiveStore& narrativeStore) {
    PlaythroughArchiveSlot& slot = archive.slots[0];
    if (characterName != nullptr) {
        std::strncpy(slot.characterName, characterName, sizeof(slot.characterName) - 1);
        slot.characterName[sizeof(slot.characterName) - 1] = '\0';
    }
    slot.worldSeed = worldSeed;
    slot.totalDaysElapsed = totalDaysElapsed;
    slot.finalCashCents = cashCents;
    slot.narrativeBeatCount = narrativeStore.beatCount;
    for (int32_t beatIndex = 0; beatIndex < MAX_NARRATIVE_BEAT_COUNT; ++beatIndex) {
        slot.narrativeBeats[beatIndex] = NarrativeBeatRecord{};
    }
    const int32_t copyCount = narrativeStore.beatCount < MAX_NARRATIVE_BEAT_COUNT ? narrativeStore.beatCount : MAX_NARRATIVE_BEAT_COUNT;
    for (int32_t beatIndex = 0; beatIndex < copyCount; ++beatIndex) {
        slot.narrativeBeats[beatIndex] = narrativeStore.beats[beatIndex];
    }
    slot.isOccupied = 1U;
}

} // namespace Core
