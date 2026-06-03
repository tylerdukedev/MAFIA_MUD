#pragma once

#include "sim/character_agent.h"
#include <cstdint>

namespace Core {

constexpr int32_t HEALTH_VALUE_MIN = 0;
constexpr int32_t HEALTH_VALUE_MAX = 100;
constexpr int32_t HEALTH_ILLNESS_THRESHOLD = 35;
constexpr int32_t PLAYER_HEALTH_TICK_INTERVAL = 200;

struct PlayerHealthStore {
    int32_t healthScore = 88;
    int32_t illnessSeverity = 0;
    bool hasHealthCoverage = false;
    uint64_t lastHealthTick = 0;
};

struct AgentHealthSlot {
    int32_t healthScore = 80;
    int32_t illnessSeverity = 0;
    bool isDeceased = false;
};

struct PopulationHealthStore {
    AgentHealthSlot agentHealth[MAX_CHARACTER_AGENT_COUNT]{};
    float boroughIllnessPressure = 0.0f;
};

void resetPlayerHealthStore(PlayerHealthStore& store);
void resetPopulationHealthStore(PopulationHealthStore& store);
void tickPlayerHealth(PlayerHealthStore& store, uint64_t tickCount, uint64_t worldSeed);
void tickPopulationHealth(PopulationHealthStore& store, CharacterAgentStore& agentStore, uint64_t tickCount, uint64_t worldSeed);
bool isPlayerCriticallyIll(const PlayerHealthStore& store);
const char* playerHealthStatusLabel(const PlayerHealthStore& store);

} // namespace Core
