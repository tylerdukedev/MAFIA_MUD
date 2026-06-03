#pragma once

#include <cstdint>

namespace Core {

enum class SimEventType : uint8_t {
    None = 0,
    ClaimCity = 1,
    EstablishOperation = 2,
    ApplyForJob = 3,
};

struct SimEvent {
    SimEventType type = SimEventType::None;
    int32_t landmarkIndex = -1;
    int32_t catalogIndex = -1;
    int32_t businessNodeIndex = -1;
};

constexpr int32_t SIM_EVENT_QUEUE_CAPACITY = 64;

struct SimEventQueue {
    SimEvent events[SIM_EVENT_QUEUE_CAPACITY];
    int32_t headIndex = 0;
    int32_t tailIndex = 0;
    int32_t eventCount = 0;
};

bool pushSimEvent(SimEventQueue& queue, SimEventType type, int32_t landmarkIndex);
bool pushSimEventWithCatalog(SimEventQueue& queue, SimEventType type, int32_t catalogIndex);
bool pushSimEventWithJob(SimEventQueue& queue, int32_t businessNodeIndex);
bool popSimEvent(SimEventQueue& queue, SimEvent& outEvent);
void clearSimEventQueue(SimEventQueue& queue);

} // namespace Core
