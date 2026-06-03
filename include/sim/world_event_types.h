#pragma once

#include <cstdint>

namespace Core {

constexpr int32_t MAX_WORLD_EVENT_DEFINITION_COUNT = 48;
constexpr int32_t MAX_WORLD_EVENT_WATCH_COUNT = 16;
constexpr int32_t MAX_WORLD_EVENT_MESSAGE_LENGTH = 128;
constexpr int32_t WORLD_EVENT_RANDOM_ROLL_INTERVAL_TICKS = 400;
constexpr int32_t WORLD_EVENT_CONDITION_SCAN_INTERVAL_TICKS = 20;
constexpr int32_t RENT_MULTIPLIER_BPS_BASE = 10000;
constexpr int32_t MISSED_RENT_MONTHS_BEFORE_EVICTION = 2;
constexpr int32_t LANDLORD_EVICTION_OPINION_THRESHOLD = -50;
constexpr int32_t FAMILY_DPA_EVICTION_OPINION_THRESHOLD = -25;

enum class WorldEventReplayPolicy : uint8_t {
    OncePerCampaign = 0,
    Repeatable = 1,
    CooldownRepeatable = 2,
};

enum class WorldEventTriggerKind : uint8_t {
    None = 0,
    Random = 1,
    Condition = 2,
    Watch = 3,
};

enum class WorldEventAudience : uint8_t {
    Player = 0,
    Agent = 1,
    Any = 2,
};

enum class WorldEventConditionKind : uint8_t {
    None = 0,
    WorldFlagSet = 1,
    AgentInactive = 2,
    AgentOpinionAtMost = 3,
    BoroughHealthAtMost = 4,
    BoroughHealthAtLeast = 5,
    PlayerMissedRentMonthsAtLeast = 6,
    PlayerHasPaidHousing = 7,
    PlayerHeadquartersKindEquals = 8,
};

enum class WorldEventEffectKind : uint8_t {
    None = 0,
    SetWorldFlag = 1,
    ClearWorldFlag = 2,
    NotifyPlayer = 3,
    EvictPlayerHeadquarters = 4,
    AdjustRentMultiplierBps = 5,
    DeactivateAgent = 6,
    AdjustAgentOpinion = 7,
    StartWatchForFlag = 8,
};

enum class WorldEventFlag : uint32_t {
    None = 0,
    MobBossEliminated = 1U << 0,
    PowerVacuum = 1U << 1,
    EconomicDistress = 1U << 2,
    LandlordHostile = 1U << 3,
};

enum class WorldEventFlagBit : uint8_t {
    MobBossEliminated = 0,
    PowerVacuum = 1,
    EconomicDistress = 2,
    LandlordHostile = 3,
};

struct WorldEventCondition {
    WorldEventConditionKind kind = WorldEventConditionKind::None;
    uint32_t flagMask = 0;
    int32_t agentSlotIndex = -1;
    int32_t threshold = 0;
    uint8_t headquartersKind = 0;
};

struct WorldEventEffect {
    WorldEventEffectKind kind = WorldEventEffectKind::None;
    uint32_t flagMask = 0;
    int32_t intParam0 = 0;
    int32_t intParam1 = 0;
    const char* message = nullptr;
};

struct WorldEventDefinition {
    const char* id;
    const char* title;
    WorldEventReplayPolicy replayPolicy;
    WorldEventTriggerKind triggerKind;
    WorldEventAudience audience;
    int32_t randomWeight;
    int32_t cooldownTicks;
    WorldEventCondition condition;
    WorldEventEffect effects[4];
};

struct WorldEventWatch {
    int32_t definitionIndex = -1;
    uint32_t awaitedFlagMask = 0;
    bool isActive = false;
};

} // namespace Core
