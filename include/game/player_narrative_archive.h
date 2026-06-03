#pragma once

#include "game/action_reason_catalog.h"
#include "game/player_law_intel.h"
#include <cstdint>

namespace Core {

constexpr int32_t MAX_NARRATIVE_BEAT_COUNT = 64;
constexpr int32_t NARRATIVE_HEADLINE_MAX_LENGTH = 96;

struct NarrativeBeatRecord {
    CovertActionKind actionKind = CovertActionKind::BribePolice;
    ActionReasonId reasonId = ActionReasonId::None;
    int32_t targetAgentIndex = -1;
    int32_t calendarDayIndex = 0;
    uint64_t tickCount = 0;
    char headline[NARRATIVE_HEADLINE_MAX_LENGTH]{};
    char narrativeTag[32]{};
};

struct PlayerNarrativeArchiveStore {
    int32_t beatCount = 0;
    NarrativeBeatRecord beats[MAX_NARRATIVE_BEAT_COUNT]{};
};

void resetPlayerNarrativeArchiveStore(PlayerNarrativeArchiveStore& store);
bool appendNarrativeBeat(
    PlayerNarrativeArchiveStore& store,
    CovertActionKind actionKind,
    ActionReasonId reasonId,
    int32_t targetAgentIndex,
    int32_t calendarDayIndex,
    uint64_t tickCount,
    const char* targetDisplayName);
int32_t getNarrativeBeatCount(const PlayerNarrativeArchiveStore& store);

} // namespace Core
