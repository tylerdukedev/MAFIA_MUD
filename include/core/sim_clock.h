#pragma once

#include <cstdint>

namespace Core {

constexpr int32_t MAX_SIM_TICKS_PER_FRAME = 32;

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
    void restoreSnapshot(uint64_t inputTickCount, bool inputPaused, double inputSpeedMultiplier, double inputAccumulatorSeconds);

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
