#pragma once

#include <cstdint>

namespace Core {

constexpr double CUSTODY_TIMER_MIN_SECONDS = 60.0;
constexpr double CUSTODY_TIMER_MAX_SECONDS = 180.0;

enum class CustodyTimerPhase : uint8_t {
    None = 0,
    PostArrestToArraignment = 1,
    ArraignmentToTrial = 2,
};

struct CustodyTimer {
    uint8_t phase = 0;
    uint8_t isActive = 0;
    double durationSeconds = 0.0;
    double elapsedSeconds = 0.0;
};

void resetCustodyTimer(CustodyTimer& timer);
void startCustodyTimer(CustodyTimer& timer, CustodyTimerPhase phase, uint64_t worldSeed, uint64_t tickCount, int32_t salt);
void tickCustodyTimer(CustodyTimer& timer, double deltaSeconds, bool isSimPaused);
bool isCustodyTimerComplete(const CustodyTimer& timer);
double getCustodyTimerRemainingSeconds(const CustodyTimer& timer);
const char* custodyTimerPhaseToString(CustodyTimerPhase phase);

} // namespace Core
