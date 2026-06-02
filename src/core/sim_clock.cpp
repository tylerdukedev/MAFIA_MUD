#include "core/sim_clock.h"

namespace Core {

SimClock::SimClock(double inputTickRateHz)
    : tickRateHz(inputTickRateHz)
    , tickIntervalSeconds(1.0 / inputTickRateHz)
    , accumulatorSeconds(0.0)
    , tickCount(0)
    , paused(true)
    , stepRequested(false)
    , ticksThisFrame(0) {
}

void SimClock::update(double deltaSeconds) {
    ticksThisFrame = 0;
    if (paused && !stepRequested) {
        return;
    }
    if (stepRequested) {
        stepRequested = false;
        advanceTick();
        return;
    }
    accumulatorSeconds += deltaSeconds;
    while (accumulatorSeconds >= tickIntervalSeconds) {
        accumulatorSeconds -= tickIntervalSeconds;
        advanceTick();
    }
}

void SimClock::setPaused(bool inputPaused) {
    paused = inputPaused;
}

void SimClock::togglePaused() {
    paused = !paused;
}

void SimClock::stepOneTick() {
    stepRequested = true;
}

bool SimClock::isPaused() const {
    return paused;
}

double SimClock::getTickRateHz() const {
    return tickRateHz;
}

uint64_t SimClock::getTickCount() const {
    return tickCount;
}

double SimClock::getAccumulatorSeconds() const {
    return accumulatorSeconds;
}

int32_t SimClock::getTicksThisFrame() const {
    return ticksThisFrame;
}

void SimClock::advanceTick() {
    ++tickCount;
    ++ticksThisFrame;
}

} // namespace Core
