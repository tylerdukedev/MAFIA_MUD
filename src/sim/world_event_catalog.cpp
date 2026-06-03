#include "sim/world_event_catalog.h"
#include "character/character_social_network.h"
#include "game/operation_types.h"
#include <cstring>

namespace Core {

namespace {

constexpr int32_t RIVAL_AGENT_SLOT_INDEX = FIRST_COMMUNITY_AGENT_SLOT_INDEX + 3;
constexpr int32_t LANDLORD_AGENT_SLOT_INDEX = FIRST_COMMUNITY_AGENT_SLOT_INDEX;

WorldEventCondition makeFlagCondition(uint32_t flagMask) {
    WorldEventCondition condition{};
    condition.kind = WorldEventConditionKind::WorldFlagSet;
    condition.flagMask = flagMask;
    return condition;
}

WorldEventCondition makeAgentInactiveCondition(int32_t agentSlotIndex) {
    WorldEventCondition condition{};
    condition.kind = WorldEventConditionKind::AgentInactive;
    condition.agentSlotIndex = agentSlotIndex;
    return condition;
}

WorldEventEffect makeNotifyEffect(const char* message) {
    WorldEventEffect effect{};
    effect.kind = WorldEventEffectKind::NotifyPlayer;
    effect.message = message;
    return effect;
}

WorldEventEffect makeSetFlagEffect(WorldEventFlag flag) {
    WorldEventEffect effect{};
    effect.kind = WorldEventEffectKind::SetWorldFlag;
    effect.flagMask = static_cast<uint32_t>(flag);
    return effect;
}

WorldEventEffect makeEvictEffect() {
    WorldEventEffect effect{};
    effect.kind = WorldEventEffectKind::EvictPlayerHeadquarters;
    return effect;
}

const WorldEventDefinition WORLD_EVENT_DEFINITIONS[] = {
    {
        "watch_mob_boss_eliminated",
        "Watching the streets after the boss falls",
        WorldEventReplayPolicy::OncePerCampaign,
        WorldEventTriggerKind::Watch,
        WorldEventAudience::Any,
        0,
        0,
        makeAgentInactiveCondition(RIVAL_AGENT_SLOT_INDEX),
        {
            makeSetFlagEffect(WorldEventFlag::MobBossEliminated),
            makeNotifyEffect("Word on the street: the old mob boss is gone. Power will shift."),
            {},
            {},
        },
    },
    {
        "mob_boss_power_vacuum",
        "Power vacuum",
        WorldEventReplayPolicy::OncePerCampaign,
        WorldEventTriggerKind::Condition,
        WorldEventAudience::Player,
        0,
        0,
        makeFlagCondition(static_cast<uint32_t>(WorldEventFlag::MobBossEliminated)),
        {
            makeSetFlagEffect(WorldEventFlag::PowerVacuum),
            makeNotifyEffect("A power vacuum opens. Rackets and families will scramble."),
            {},
            {},
        },
    },
    {
        "random_economic_slump",
        "Local slump",
        WorldEventReplayPolicy::CooldownRepeatable,
        WorldEventTriggerKind::Random,
        WorldEventAudience::Any,
        14,
        2400,
        {},
        {
            makeSetFlagEffect(WorldEventFlag::EconomicDistress),
            makeNotifyEffect("A borough slump tightens purses. Rents may dip, but wages sag too."),
            {WorldEventEffectKind::AdjustRentMultiplierBps, 0, -600, 0, nullptr},
            {},
        },
    },
    {
        "random_economic_boom",
        "Building boom",
        WorldEventReplayPolicy::CooldownRepeatable,
        WorldEventTriggerKind::Random,
        WorldEventAudience::Any,
        10,
        2800,
        {},
        {
            {WorldEventEffectKind::ClearWorldFlag, static_cast<uint32_t>(WorldEventFlag::EconomicDistress), 0, 0, nullptr},
            {WorldEventEffectKind::AdjustRentMultiplierBps, 0, 500, 0, nullptr},
            makeNotifyEffect("Construction and trade pick up. Landlords push rents higher."),
            {},
        },
    },
    {
        "eviction_missed_rent",
        "Evicted for unpaid rent",
        WorldEventReplayPolicy::Repeatable,
        WorldEventTriggerKind::Condition,
        WorldEventAudience::Player,
        0,
        800,
        {WorldEventConditionKind::PlayerMissedRentMonthsAtLeast, 0, -1, MISSED_RENT_MONTHS_BEFORE_EVICTION, 0},
        {
            makeEvictEffect(),
            makeNotifyEffect("You are evicted. The landlord padlocks the door."),
            {},
            {},
        },
    },
    {
        "eviction_landlord_hostile",
        "Landlord throws you out",
        WorldEventReplayPolicy::CooldownRepeatable,
        WorldEventTriggerKind::Condition,
        WorldEventAudience::Player,
        0,
        1200,
        {WorldEventConditionKind::AgentOpinionAtMost, 0, LANDLORD_AGENT_SLOT_INDEX, LANDLORD_EVICTION_OPINION_THRESHOLD, static_cast<uint8_t>(HeadquartersKind::RentedRoom)},
        {
            makeEvictEffect(),
            makeNotifyEffect("The landlord has had enough. Your rented room is finished."),
            {WorldEventEffectKind::SetWorldFlag, static_cast<uint32_t>(WorldEventFlag::LandlordHostile), 0, 0, nullptr},
            {},
        },
    },
    {
        "eviction_family_dpa",
        "Family asks you to leave",
        WorldEventReplayPolicy::CooldownRepeatable,
        WorldEventTriggerKind::Condition,
        WorldEventAudience::Player,
        0,
        1600,
        {WorldEventConditionKind::AgentOpinionAtMost, 0, FAMILY_AGENT_SLOT_INDEX, FAMILY_DPA_EVICTION_OPINION_THRESHOLD, static_cast<uint8_t>(HeadquartersKind::FamilyFriendDpa)},
        {
            makeEvictEffect(),
            makeNotifyEffect("Your family will not host you anymore. Find a roof."),
            {},
            {},
        },
    },
};

} // namespace

int32_t getWorldEventDefinitionCount() {
    return static_cast<int32_t>(sizeof(WORLD_EVENT_DEFINITIONS) / sizeof(WORLD_EVENT_DEFINITIONS[0]));
}

const WorldEventDefinition* getWorldEventDefinition(int32_t definitionIndex) {
    const int32_t count = getWorldEventDefinitionCount();
    if (definitionIndex < 0 || definitionIndex >= count) {
        return nullptr;
    }
    return &WORLD_EVENT_DEFINITIONS[definitionIndex];
}

int32_t findWorldEventDefinitionIndexById(const char* eventId) {
    if (eventId == nullptr) {
        return -1;
    }
    const int32_t count = getWorldEventDefinitionCount();
    for (int32_t index = 0; index < count; ++index) {
        if (std::strcmp(WORLD_EVENT_DEFINITIONS[index].id, eventId) == 0) {
            return index;
        }
    }
    return -1;
}

} // namespace Core
