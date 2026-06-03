#include "sim/sim_event_queue.h"

namespace Core {

bool pushSimEventWithCatalog(SimEventQueue& queue, SimEventType type, int32_t catalogIndex) {
    if (queue.eventCount >= SIM_EVENT_QUEUE_CAPACITY) {
        return false;
    }
    SimEvent& event = queue.events[queue.tailIndex];
    event.type = type;
    event.landmarkIndex = -1;
    event.catalogIndex = catalogIndex;
    event.businessNodeIndex = -1;
    queue.tailIndex = (queue.tailIndex + 1) % SIM_EVENT_QUEUE_CAPACITY;
    ++queue.eventCount;
    return true;
}

bool pushSimEventWithJob(SimEventQueue& queue, int32_t businessNodeIndex) {
    if (queue.eventCount >= SIM_EVENT_QUEUE_CAPACITY) {
        return false;
    }
    SimEvent& event = queue.events[queue.tailIndex];
    event.type = SimEventType::ApplyForJob;
    event.landmarkIndex = -1;
    event.catalogIndex = -1;
    event.businessNodeIndex = businessNodeIndex;
    queue.tailIndex = (queue.tailIndex + 1) % SIM_EVENT_QUEUE_CAPACITY;
    ++queue.eventCount;
    return true;
}

bool pushSimEvent(SimEventQueue& queue, SimEventType type, int32_t landmarkIndex) {
    if (queue.eventCount >= SIM_EVENT_QUEUE_CAPACITY) {
        return false;
    }
    SimEvent& event = queue.events[queue.tailIndex];
    event.type = type;
    event.landmarkIndex = landmarkIndex;
    event.catalogIndex = -1;
    event.businessNodeIndex = -1;
    queue.tailIndex = (queue.tailIndex + 1) % SIM_EVENT_QUEUE_CAPACITY;
    ++queue.eventCount;
    return true;
}

bool pushSimEventRestored(SimEventQueue& queue, const SimEvent& event) {
    if (queue.eventCount >= SIM_EVENT_QUEUE_CAPACITY) {
        return false;
    }
    queue.events[queue.tailIndex] = event;
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
