#include "game/player_health.h"
#include <algorithm>
#include <cstring>

namespace Core {

void resetPlayerHealthStore(PlayerHealthStore& store) {
    store.healthScore = 88;
    store.illnessSeverity = 0;
    store.hasHealthCoverage = false;
    store.lastHealthTick = 0;
}

void resetPopulationHealthStore(PopulationHealthStore& store) {
    for (int32_t agentIndex = 0; agentIndex < MAX_CHARACTER_AGENT_COUNT; ++agentIndex) {
        store.agentHealth[agentIndex] = AgentHealthSlot{};
        store.agentHealth[agentIndex].healthScore = 70 + (agentIndex * 3) % 25;
    }
    store.boroughIllnessPressure = 0.0f;
}

bool isPlayerCriticallyIll(const PlayerHealthStore& store) {
    return store.healthScore < HEALTH_ILLNESS_THRESHOLD || store.illnessSeverity >= 50;
}

const char* playerHealthStatusLabel(const PlayerHealthStore& store) {
    if (store.illnessSeverity >= 60) {
        return "Seriously ill";
    }
    if (store.illnessSeverity >= 25) {
        return "Under the weather";
    }
    if (store.healthScore >= 80) {
        return "Healthy";
    }
    return "Worn down";
}

void tickPlayerHealth(PlayerHealthStore& store, uint64_t tickCount, uint64_t worldSeed) {
    if (store.lastHealthTick != 0ULL && tickCount - store.lastHealthTick < static_cast<uint64_t>(PLAYER_HEALTH_TICK_INTERVAL)) {
        return;
    }
    store.lastHealthTick = tickCount;
    const uint32_t roll = static_cast<uint32_t>((worldSeed ^ tickCount) % 1000ULL);
    if (roll < 3U && store.illnessSeverity < 80) {
        store.illnessSeverity += 4;
        store.healthScore = std::max(HEALTH_VALUE_MIN, store.healthScore - 3);
        return;
    }
    if (store.illnessSeverity > 0 && roll < 120U) {
        store.illnessSeverity = std::max(0, store.illnessSeverity - 2);
    }
    if (store.healthScore < 95 && roll < 80U) {
        store.healthScore = std::min(HEALTH_VALUE_MAX, store.healthScore + 1);
    }
}

void tickPopulationHealth(PopulationHealthStore& store, CharacterAgentStore& agentStore, uint64_t tickCount, uint64_t worldSeed) {
    if (tickCount % static_cast<uint64_t>(PLAYER_HEALTH_TICK_INTERVAL) != 0ULL) {
        return;
    }
    int32_t illCount = 0;
    for (int32_t agentIndex = 0; agentIndex < MAX_CHARACTER_AGENT_COUNT; ++agentIndex) {
        if (!agentStore.states[agentIndex].isActive) {
            continue;
        }
        AgentHealthSlot& slot = store.agentHealth[agentIndex];
        if (slot.isDeceased) {
            continue;
        }
        const uint32_t roll = static_cast<uint32_t>((worldSeed ^ tickCount ^ static_cast<uint64_t>(agentIndex)) % 2000ULL);
        if (roll < 2U) {
            slot.illnessSeverity = std::min(100, slot.illnessSeverity + 5);
            slot.healthScore = std::max(0, slot.healthScore - 4);
            illCount += 1;
        }
        if (roll < 30U && slot.illnessSeverity > 0) {
            slot.illnessSeverity = std::max(0, slot.illnessSeverity - 1);
        }
        if (roll < 20U && slot.healthScore < 100) {
            slot.healthScore = std::min(100, slot.healthScore + 1);
        }
        if (slot.healthScore <= 0 && slot.illnessSeverity >= 95) {
            slot.isDeceased = true;
            agentStore.states[agentIndex].isActive = false;
        }
    }
    store.boroughIllnessPressure = static_cast<float>(illCount) * 0.04f;
}

} // namespace Core
