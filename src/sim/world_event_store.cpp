#include "sim/world_event_store.h"
#include <cstring>

namespace Core {

void resetWorldEventStore(WorldEventStore& store) {
    store.worldFlags = 0;
    store.firedOnceMask = 0;
    store.lastRandomRollTick = 0;
    store.lastConditionScanTick = 0;
    store.lastPlayerMessage[0] = '\0';
    for (int32_t index = 0; index < MAX_WORLD_EVENT_DEFINITION_COUNT; ++index) {
        store.lastFiredTick[index] = 0;
        store.repeatFireCount[index] = 0;
    }
    for (int32_t watchIndex = 0; watchIndex < MAX_WORLD_EVENT_WATCH_COUNT; ++watchIndex) {
        store.watches[watchIndex] = WorldEventWatch{};
    }
}

bool isWorldEventFlagSet(const WorldEventStore& store, WorldEventFlag flag) {
    return (store.worldFlags & static_cast<uint32_t>(flag)) != 0U;
}

void setWorldEventFlag(WorldEventStore& store, WorldEventFlag flag) {
    store.worldFlags |= static_cast<uint32_t>(flag);
}

void clearWorldEventFlag(WorldEventStore& store, WorldEventFlag flag) {
    store.worldFlags &= ~static_cast<uint32_t>(flag);
}

bool hasWorldEventFiredOnce(const WorldEventStore& store, int32_t definitionIndex) {
    if (definitionIndex < 0 || definitionIndex >= 64) {
        return false;
    }
    return (store.firedOnceMask & (1ULL << static_cast<uint64_t>(definitionIndex))) != 0ULL;
}

void markWorldEventFired(WorldEventStore& store, int32_t definitionIndex, uint64_t tickCount) {
    if (definitionIndex < 0 || definitionIndex >= MAX_WORLD_EVENT_DEFINITION_COUNT) {
        return;
    }
    if (definitionIndex < 64) {
        store.firedOnceMask |= 1ULL << static_cast<uint64_t>(definitionIndex);
    }
    store.lastFiredTick[definitionIndex] = tickCount;
    ++store.repeatFireCount[definitionIndex];
}

bool canFireWorldEventByReplayPolicy(
    const WorldEventStore& store,
    const WorldEventDefinition& definition,
    int32_t definitionIndex,
    uint64_t tickCount) {
    if (definition.replayPolicy == WorldEventReplayPolicy::OncePerCampaign) {
        return !hasWorldEventFiredOnce(store, definitionIndex);
    }
    if (definition.replayPolicy == WorldEventReplayPolicy::Repeatable) {
        return true;
    }
    if (definition.replayPolicy == WorldEventReplayPolicy::CooldownRepeatable) {
        if (definition.cooldownTicks <= 0) {
            return true;
        }
        const uint64_t lastTick = store.lastFiredTick[definitionIndex];
        if (lastTick == 0ULL) {
            return true;
        }
        return tickCount - lastTick >= static_cast<uint64_t>(definition.cooldownTicks);
    }
    return false;
}

void publishWorldEventMessage(WorldEventStore& store, const char* message) {
    if (message == nullptr) {
        return;
    }
    std::strncpy(store.lastPlayerMessage, message, MAX_WORLD_EVENT_MESSAGE_LENGTH - 1);
    store.lastPlayerMessage[MAX_WORLD_EVENT_MESSAGE_LENGTH - 1] = '\0';
}

} // namespace Core
