#include "core/sim_clock.h"

namespace Core {

SimClock::SimClock(double inputTickRateHz)
    : tickRateHz(inputTickRateHz)
    , tickIntervalSeconds(1.0 / inputTickRateHz)
    , accumulatorSeconds(0.0)
    , speedMultiplier(1.0)
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
    accumulatorSeconds += deltaSeconds * speedMultiplier;
    while (accumulatorSeconds >= tickIntervalSeconds && ticksThisFrame < MAX_SIM_TICKS_PER_FRAME) {
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

void SimClock::setSpeedMultiplier(double multiplier) {
    if (multiplier < 0.0) {
        speedMultiplier = 0.0;
        return;
    }
    speedMultiplier = multiplier;
}

double SimClock::getSpeedMultiplier() const {
    return speedMultiplier;
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

void SimClock::restoreSnapshot(uint64_t inputTickCount, bool inputPaused, double inputSpeedMultiplier, double inputAccumulatorSeconds) {
    tickCount = inputTickCount;
    paused = inputPaused;
    speedMultiplier = inputSpeedMultiplier < 0.0 ? 0.0 : inputSpeedMultiplier;
    accumulatorSeconds = inputAccumulatorSeconds < 0.0 ? 0.0 : inputAccumulatorSeconds;
    stepRequested = false;
    ticksThisFrame = 0;
}

} // namespace Core
