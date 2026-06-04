#include "game/jail_events.h"
#include "utils/seed_hash.h"
#include <cstring>

namespace Core {

namespace {

constexpr int32_t JAIL_EVENT_TYPE_COUNT = 5;

void setDescription(JailEvent& event, const char* text) {
    std::strncpy(event.description, text, JAIL_EVENT_DESCRIPTION_LENGTH - 1);
    event.description[JAIL_EVENT_DESCRIPTION_LENGTH - 1] = '\0';
}

void buildFightEvent(JailEvent& event) {
    event.type = JailEventType::Fight;
    event.heatDelta = 3;
    setDescription(event, "A scuffle broke out in the yard. You held your own, but the guards noticed.");
}

void buildIntelEvent(JailEvent& event) {
    event.type = JailEventType::IntelGathered;
    event.evidenceDelta = -4;
    setDescription(event, "A cellmate let slip which witness the DA is leaning on. Useful leverage.");
}

void buildAllyEvent(JailEvent& event) {
    event.type = JailEventType::AllyMade;
    setDescription(event, "You did a favor for a connected inmate. He says he owes you on the outside.");
}

void buildShakedownEvent(JailEvent& event) {
    event.type = JailEventType::Shakedown;
    event.cashDelta = -JAIL_SHAKEDOWN_LOSS_CENTS;
    setDescription(event, "A dirty guard shook you down for commissary money. You paid to avoid trouble.");
}

void buildMessageEvent(JailEvent& event) {
    event.type = JailEventType::Message;
    setDescription(event, "Word arrived from the outside: your crew is holding the corner until you return.");
}

} // namespace

bool tryFireJailEvent(
    JailEvent& outEvent,
    uint64_t custodyStartedTick,
    uint64_t worldSeed,
    uint64_t tickCount) {
    if (tickCount % static_cast<uint64_t>(JAIL_EVENT_INTERVAL_TICKS) != 0ULL) {
        return false;
    }
    const uint32_t chanceRoll = Utils::hashSeedMix(worldSeed, static_cast<int32_t>(tickCount), 0x4A41494C) % 100U;
    if (static_cast<int32_t>(chanceRoll) >= JAIL_EVENT_CHANCE_PERCENT) {
        return false;
    }
    outEvent = JailEvent{};
    const uint32_t typeRoll = Utils::hashSeedMix(worldSeed, static_cast<int32_t>(custodyStartedTick), static_cast<int32_t>(tickCount)) % static_cast<uint32_t>(JAIL_EVENT_TYPE_COUNT);
    switch (static_cast<JailEventType>(typeRoll)) {
        case JailEventType::Fight:         buildFightEvent(outEvent); break;
        case JailEventType::IntelGathered: buildIntelEvent(outEvent); break;
        case JailEventType::AllyMade:      buildAllyEvent(outEvent); break;
        case JailEventType::Shakedown:     buildShakedownEvent(outEvent); break;
        case JailEventType::Message:       buildMessageEvent(outEvent); break;
    }
    return true;
}

const char* jailEventTypeToString(JailEventType type) {
    switch (type) {
        case JailEventType::Fight:         return "Yard Fight";
        case JailEventType::IntelGathered: return "Intel Gathered";
        case JailEventType::AllyMade:      return "Ally Made";
        case JailEventType::Shakedown:     return "Shakedown";
        case JailEventType::Message:       return "Message";
    }
    return "Jail Event";
}

} // namespace Core
