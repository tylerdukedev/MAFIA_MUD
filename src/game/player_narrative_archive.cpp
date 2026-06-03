#include "game/player_narrative_archive.h"
#include <cstdio>
#include <cstring>

namespace Core {

namespace {

void formatBeatHeadline(
    char* outBuffer,
    size_t bufferSize,
    CovertActionKind actionKind,
    ActionReasonId reasonId,
    const char* targetDisplayName) {
    const ActionReasonDefinition* definition = getActionReasonDefinition(reasonId);
    const char* reasonLabel = definition != nullptr ? definition->label : "unknown motive";
    const char* actionLabel = covertActionKindToLabel(actionKind);
    if (targetDisplayName != nullptr && targetDisplayName[0] != '\0') {
        std::snprintf(outBuffer, bufferSize, "%s — %s (%s)", actionLabel, reasonLabel, targetDisplayName);
        return;
    }
    std::snprintf(outBuffer, bufferSize, "%s — %s", actionLabel, reasonLabel);
}

} // namespace

void resetPlayerNarrativeArchiveStore(PlayerNarrativeArchiveStore& store) {
    store.beatCount = 0;
    for (int32_t beatIndex = 0; beatIndex < MAX_NARRATIVE_BEAT_COUNT; ++beatIndex) {
        store.beats[beatIndex] = NarrativeBeatRecord{};
    }
}

bool appendNarrativeBeat(
    PlayerNarrativeArchiveStore& store,
    CovertActionKind actionKind,
    ActionReasonId reasonId,
    int32_t targetAgentIndex,
    int32_t calendarDayIndex,
    uint64_t tickCount,
    const char* targetDisplayName) {
    if (store.beatCount >= MAX_NARRATIVE_BEAT_COUNT) {
        return false;
    }
    NarrativeBeatRecord& beat = store.beats[store.beatCount];
    beat.actionKind = actionKind;
    beat.reasonId = reasonId;
    beat.targetAgentIndex = targetAgentIndex;
    beat.calendarDayIndex = calendarDayIndex;
    beat.tickCount = tickCount;
    formatBeatHeadline(beat.headline, sizeof(beat.headline), actionKind, reasonId, targetDisplayName);
    const ActionReasonDefinition* definition = getActionReasonDefinition(reasonId);
    if (definition != nullptr) {
        std::snprintf(beat.narrativeTag, sizeof(beat.narrativeTag), "%s", definition->narrativeTag);
    }
    store.beatCount += 1;
    return true;
}

int32_t getNarrativeBeatCount(const PlayerNarrativeArchiveStore& store) {
    return store.beatCount;
}

} // namespace Core
