#pragma once

#include <cstdint>

namespace Core {

constexpr int32_t JAIL_EVENT_INTERVAL_TICKS = 12;
constexpr int32_t JAIL_EVENT_CHANCE_PERCENT = 45;
constexpr int32_t JAIL_EVENT_DESCRIPTION_LENGTH = 128;
constexpr int64_t JAIL_SHAKEDOWN_LOSS_CENTS = 1500;

enum class JailEventType : uint8_t {
    Fight = 0,
    IntelGathered = 1,
    AllyMade = 2,
    Shakedown = 3,
    Message = 4,
};

struct JailEvent {
    JailEventType type = JailEventType::Message;
    char description[JAIL_EVENT_DESCRIPTION_LENGTH]{};
    int32_t heatDelta = 0;
    int32_t evidenceDelta = 0;
    int64_t cashDelta = 0;
    int32_t allyAgentIndex = -1;
};

bool tryFireJailEvent(
    JailEvent& outEvent,
    uint64_t custodyStartedTick,
    uint64_t worldSeed,
    uint64_t tickCount);
const char* jailEventTypeToString(JailEventType type);

} // namespace Core
