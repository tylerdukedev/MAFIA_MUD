#pragma once

#include "sim/world_event_types.h"
#include <cstdint>

namespace Core {

struct WorldEventStore {
    uint32_t worldFlags = 0;
    uint64_t firedOnceMask = 0;
    uint64_t lastFiredTick[MAX_WORLD_EVENT_DEFINITION_COUNT]{};
    int32_t repeatFireCount[MAX_WORLD_EVENT_DEFINITION_COUNT]{};
    WorldEventWatch watches[MAX_WORLD_EVENT_WATCH_COUNT]{};
    char lastPlayerMessage[MAX_WORLD_EVENT_MESSAGE_LENGTH]{};
    uint64_t lastRandomRollTick = 0;
    uint64_t lastConditionScanTick = 0;
};

void resetWorldEventStore(WorldEventStore& store);
bool isWorldEventFlagSet(const WorldEventStore& store, WorldEventFlag flag);
void setWorldEventFlag(WorldEventStore& store, WorldEventFlag flag);
void clearWorldEventFlag(WorldEventStore& store, WorldEventFlag flag);
bool hasWorldEventFiredOnce(const WorldEventStore& store, int32_t definitionIndex);
void markWorldEventFired(WorldEventStore& store, int32_t definitionIndex, uint64_t tickCount);
bool canFireWorldEventByReplayPolicy(
    const WorldEventStore& store,
    const WorldEventDefinition& definition,
    int32_t definitionIndex,
    uint64_t tickCount);
void publishWorldEventMessage(WorldEventStore& store, const char* message);

} // namespace Core
