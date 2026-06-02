#pragma once

#include <cstdint>

namespace Core {

class SimClock {
public:
    explicit SimClock(double tickRateHz = 20.0);
    void update(double deltaSeconds);
    void setPaused(bool paused);
    void togglePaused();
    void stepOneTick();
    void setSpeedMultiplier(double multiplier);
    double getSpeedMultiplier() const;
    bool isPaused() const;
    double getTickRateHz() const;
    uint64_t getTickCount() const;
    double getAccumulatorSeconds() const;
    int32_t getTicksThisFrame() const;

private:
    double tickRateHz;
    double tickIntervalSeconds;
    double accumulatorSeconds;
    double speedMultiplier;
    uint64_t tickCount;
    bool paused;
    bool stepRequested;
    int32_t ticksThisFrame;
    void advanceTick();
};

} // namespace Core
