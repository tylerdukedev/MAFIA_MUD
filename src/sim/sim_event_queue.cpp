#include "sim/sim_event_queue.h"

namespace Core {

bool pushSimEvent(SimEventQueue& queue, SimEventType type, int32_t landmarkIndex) {
    if (queue.eventCount >= SIM_EVENT_QUEUE_CAPACITY) {
        return false;
    }
    SimEvent& event = queue.events[queue.tailIndex];
    event.type = type;
    event.landmarkIndex = landmarkIndex;
    queue.tailIndex = (queue.tailIndex + 1) % SIM_EVENT_QUEUE_CAPACITY;
    ++queue.eventCount;
    return true;
}

bool popSimEvent(SimEventQueue& queue, SimEvent& outEvent) {
    if (queue.eventCount <= 0) {
        return false;
    }
    outEvent = queue.events[queue.headIndex];
    queue.headIndex = (queue.headIndex + 1) % SIM_EVENT_QUEUE_CAPACITY;
    --queue.eventCount;
    return true;
}

void clearSimEventQueue(SimEventQueue& queue) {
    queue.headIndex = 0;
    queue.tailIndex = 0;
    queue.eventCount = 0;
}

} // namespace Core
