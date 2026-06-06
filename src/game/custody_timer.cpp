#include "game/custody_timer.h"
#include "utils/seed_hash.h"
#include <algorithm>

namespace Core {

void resetCustodyTimer(CustodyTimer& timer) {
    timer.phase = 0;
    timer.isActive = 0;
    timer.durationSeconds = 0.0;
    timer.elapsedSeconds = 0.0;
}

void startCustodyTimer(CustodyTimer& timer, CustodyTimerPhase phase, uint64_t worldSeed, uint64_t tickCount, int32_t salt) {
    const uint32_t roll = Utils::hashSeedMix(worldSeed, static_cast<int32_t>(tickCount), salt) % 121U;
    const double span = CUSTODY_TIMER_MAX_SECONDS - CUSTODY_TIMER_MIN_SECONDS;
    timer.phase = static_cast<uint8_t>(phase);
    timer.isActive = 1;
    timer.durationSeconds = CUSTODY_TIMER_MIN_SECONDS + static_cast<double>(roll) * span / 120.0;
    timer.elapsedSeconds = 0.0;
}

void tickCustodyTimer(CustodyTimer& timer, double deltaSeconds, bool isSimPaused) {
    if (timer.isActive == 0 || isSimPaused || deltaSeconds <= 0.0) {
        return;
    }
    timer.elapsedSeconds += deltaSeconds;
}

bool isCustodyTimerComplete(const CustodyTimer& timer) {
    if (timer.isActive == 0) {
        return false;
    }
    return timer.elapsedSeconds >= timer.durationSeconds;
}

double getCustodyTimerRemainingSeconds(const CustodyTimer& timer) {
    if (timer.isActive == 0) {
        return 0.0;
    }
    const double remaining = timer.durationSeconds - timer.elapsedSeconds;
    return remaining > 0.0 ? remaining : 0.0;
}

const char* custodyTimerPhaseToString(CustodyTimerPhase phase) {
    switch (phase) {
    case CustodyTimerPhase::PostArrestToArraignment:
        return "Booking to arraignment";
    case CustodyTimerPhase::ArraignmentToTrial:
        return "Arraignment to trial";
    default:
        return "Inactive";
    }
}

} // namespace Core
