#include "sim/world_event_system.h"
#include "character/character_social_network.h"
#include "game/housing_economy.h"
#include "game/player_operations.h"
#include "sim/character_agent.h"
#include "sim/world_event_catalog.h"
#include "utils/seed_hash.h"
#include <algorithm>
#include <cstring>

namespace Core {

namespace {

constexpr int32_t RANDOM_EVENT_BASE_CHANCE_PERCENT = 4;

} // namespace

bool evaluateWorldEventCondition(
    const WorldEventCondition& condition,
    const WorldEventStore& eventStore,
    const PlayerOperationsStore& operationsStore,
    const PlayerWallet& wallet,
    const CharacterAgentStore& agentStore,
    const BoroughVitalityStore& boroughVitalityStore) {
    if (condition.kind == WorldEventConditionKind::None) {
        return true;
    }
    if (condition.kind == WorldEventConditionKind::WorldFlagSet) {
        return (eventStore.worldFlags & condition.flagMask) == condition.flagMask;
    }
    if (condition.kind == WorldEventConditionKind::AgentInactive) {
        if (condition.agentSlotIndex < 0 || condition.agentSlotIndex >= MAX_CHARACTER_AGENT_COUNT) {
            return false;
        }
        return !agentStore.states[condition.agentSlotIndex].isActive;
    }
    if (condition.kind == WorldEventConditionKind::AgentOpinionAtMost) {
        if (condition.agentSlotIndex < 0 || condition.agentSlotIndex >= MAX_CHARACTER_AGENT_COUNT) {
            return false;
        }
        if (!agentStore.states[condition.agentSlotIndex].isActive) {
            return false;
        }
        if (condition.headquartersKind != 0U && static_cast<uint8_t>(operationsStore.headquartersKind) != condition.headquartersKind) {
            return false;
        }
        return agentStore.states[condition.agentSlotIndex].opinionOfPlayer <= condition.threshold;
    }
    if (condition.kind == WorldEventConditionKind::BoroughHealthAtMost) {
        const RegionId regionId = static_cast<RegionId>(operationsStore.headquartersRegionId);
        const BoroughVitalitySnapshot* snapshot = getBoroughSnapshot(boroughVitalityStore, regionId);
        if (snapshot == nullptr) {
            return false;
        }
        return snapshot->economicHealth <= static_cast<float>(condition.threshold);
    }
    if (condition.kind == WorldEventConditionKind::BoroughHealthAtLeast) {
        const RegionId regionId = static_cast<RegionId>(operationsStore.headquartersRegionId);
        const BoroughVitalitySnapshot* snapshot = getBoroughSnapshot(boroughVitalityStore, regionId);
        if (snapshot == nullptr) {
            return false;
        }
        return snapshot->economicHealth >= static_cast<float>(condition.threshold);
    }
    if (condition.kind == WorldEventConditionKind::PlayerMissedRentMonthsAtLeast) {
        return operationsStore.consecutiveUnpaidRentMonths >= static_cast<int8_t>(condition.threshold);
    }
    if (condition.kind == WorldEventConditionKind::PlayerHeadquartersKindEquals) {
        return static_cast<uint8_t>(operationsStore.headquartersKind) == condition.headquartersKind;
    }
    if (condition.kind == WorldEventConditionKind::PlayerHasPaidHousing) {
        return hasPlayerHeadquarters(operationsStore) && wallet.cashCents > 0;
    }
    (void)eventStore;
    return false;
}

void applyWorldEventEffect(
    const WorldEventEffect& effect,
    WorldEventStore& eventStore,
    PlayerOperationsStore& operationsStore,
    PlayerWallet& wallet,
    CharacterAgentStore& agentStore,
    uint64_t tickCount) {
    (void)tickCount;
    if (effect.kind == WorldEventEffectKind::SetWorldFlag) {
        eventStore.worldFlags |= effect.flagMask;
        return;
    }
    if (effect.kind == WorldEventEffectKind::ClearWorldFlag) {
        eventStore.worldFlags &= ~effect.flagMask;
        return;
    }
    if (effect.kind == WorldEventEffectKind::NotifyPlayer) {
        publishWorldEventMessage(eventStore, effect.message);
        return;
    }
    if (effect.kind == WorldEventEffectKind::EvictPlayerHeadquarters) {
        clearPlayerHeadquarters(operationsStore);
        return;
    }
    if (effect.kind == WorldEventEffectKind::AdjustRentMultiplierBps) {
        operationsStore.rentEventAdjustmentBps += effect.intParam0;
        operationsStore.rentEventAdjustmentBps = std::clamp(operationsStore.rentEventAdjustmentBps, -2500, 2500);
        return;
    }
    if (effect.kind == WorldEventEffectKind::DeactivateAgent) {
        if (effect.intParam0 >= 0 && effect.intParam0 < MAX_CHARACTER_AGENT_COUNT) {
            agentStore.states[effect.intParam0].isActive = false;
        }
        return;
    }
    if (effect.kind == WorldEventEffectKind::AdjustAgentOpinion) {
        if (effect.intParam0 >= 0 && effect.intParam0 < MAX_CHARACTER_AGENT_COUNT) {
            adjustAgentOpinion(agentStore, effect.intParam0, effect.intParam1);
        }
        return;
    }
    (void)wallet;
}

void WorldEventSystem::bind(const SimWorldBindings& inputBindings) {
    bindings = inputBindings;
    areWatchesInitialized = false;
}

const char* WorldEventSystem::getName() const {
    return "WorldEventSystem";
}

void WorldEventSystem::initializeWatches() {
    if (bindings.worldEventStore == nullptr || areWatchesInitialized) {
        return;
    }
    WorldEventStore& store = *bindings.worldEventStore;
    int32_t nextWatchSlot = 0;
    const int32_t definitionCount = getWorldEventDefinitionCount();
    for (int32_t definitionIndex = 0; definitionIndex < definitionCount; ++definitionIndex) {
        const WorldEventDefinition* definition = getWorldEventDefinition(definitionIndex);
        if (definition == nullptr || definition->triggerKind != WorldEventTriggerKind::Watch) {
            continue;
        }
        if (nextWatchSlot >= MAX_WORLD_EVENT_WATCH_COUNT) {
            break;
        }
        store.watches[nextWatchSlot].definitionIndex = definitionIndex;
        store.watches[nextWatchSlot].awaitedFlagMask = definition->condition.flagMask;
        store.watches[nextWatchSlot].isActive = true;
        ++nextWatchSlot;
    }
    areWatchesInitialized = true;
}

bool WorldEventSystem::evaluateCondition(const WorldEventDefinition& definition, uint64_t tickCount) const {
    (void)tickCount;
    if (bindings.worldEventStore == nullptr || bindings.playerOperationsStore == nullptr || bindings.playerWallet == nullptr || bindings.characterAgentStore == nullptr || bindings.boroughVitalityStore == nullptr) {
        return false;
    }
    return evaluateWorldEventCondition(
        definition.condition,
        *bindings.worldEventStore,
        *bindings.playerOperationsStore,
        *bindings.playerWallet,
        *bindings.characterAgentStore,
        *bindings.boroughVitalityStore);
}

void WorldEventSystem::applyEffect(const WorldEventEffect& effect, uint64_t tickCount) {
    if (bindings.worldEventStore == nullptr || bindings.playerOperationsStore == nullptr || bindings.playerWallet == nullptr || bindings.characterAgentStore == nullptr) {
        return;
    }
    applyWorldEventEffect(
        effect,
        *bindings.worldEventStore,
        *bindings.playerOperationsStore,
        *bindings.playerWallet,
        *bindings.characterAgentStore,
        tickCount);
}

void WorldEventSystem::tryFireDefinition(int32_t definitionIndex, uint64_t tickCount) {
    if (bindings.worldEventStore == nullptr) {
        return;
    }
    const WorldEventDefinition* definition = getWorldEventDefinition(definitionIndex);
    if (definition == nullptr) {
        return;
    }
    WorldEventStore& store = *bindings.worldEventStore;
    if (!canFireWorldEventByReplayPolicy(store, *definition, definitionIndex, tickCount)) {
        return;
    }
    if (!evaluateCondition(*definition, tickCount)) {
        return;
    }
    for (int32_t effectIndex = 0; effectIndex < 4; ++effectIndex) {
        if (definition->effects[effectIndex].kind == WorldEventEffectKind::None) {
            continue;
        }
        applyEffect(definition->effects[effectIndex], tickCount);
    }
    markWorldEventFired(store, definitionIndex, tickCount);
}

void WorldEventSystem::scanConditionEvents(uint64_t tickCount) {
    const int32_t definitionCount = getWorldEventDefinitionCount();
    for (int32_t definitionIndex = 0; definitionIndex < definitionCount; ++definitionIndex) {
        const WorldEventDefinition* definition = getWorldEventDefinition(definitionIndex);
        if (definition == nullptr || definition->triggerKind != WorldEventTriggerKind::Condition) {
            continue;
        }
        tryFireDefinition(definitionIndex, tickCount);
    }
}

void WorldEventSystem::scanWatchEvents(uint64_t tickCount) {
    if (bindings.worldEventStore == nullptr) {
        return;
    }
    WorldEventStore& store = *bindings.worldEventStore;
    for (int32_t watchIndex = 0; watchIndex < MAX_WORLD_EVENT_WATCH_COUNT; ++watchIndex) {
        WorldEventWatch& watch = store.watches[watchIndex];
        if (!watch.isActive || watch.definitionIndex < 0) {
            continue;
        }
        const WorldEventDefinition* definition = getWorldEventDefinition(watch.definitionIndex);
        if (definition == nullptr) {
            watch.isActive = false;
            continue;
        }
        if (!evaluateCondition(*definition, tickCount)) {
            continue;
        }
        tryFireDefinition(watch.definitionIndex, tickCount);
        watch.isActive = false;
    }
}

void WorldEventSystem::scanRandomEvents(uint64_t tickCount) {
    if (bindings.worldEventStore == nullptr || bindings.worldSeed == nullptr) {
        return;
    }
    WorldEventStore& store = *bindings.worldEventStore;
    if (tickCount - store.lastRandomRollTick < static_cast<uint64_t>(WORLD_EVENT_RANDOM_ROLL_INTERVAL_TICKS)) {
        return;
    }
    store.lastRandomRollTick = tickCount;
    const uint32_t roll = Utils::hashSeedMix(*bindings.worldSeed, static_cast<int32_t>(tickCount), 0x45564E54) % 100U;
    if (static_cast<int32_t>(roll) >= RANDOM_EVENT_BASE_CHANCE_PERCENT) {
        return;
    }
    int32_t totalWeight = 0;
    const int32_t definitionCount = getWorldEventDefinitionCount();
    for (int32_t definitionIndex = 0; definitionIndex < definitionCount; ++definitionIndex) {
        const WorldEventDefinition* definition = getWorldEventDefinition(definitionIndex);
        if (definition == nullptr || definition->triggerKind != WorldEventTriggerKind::Random) {
            continue;
        }
        totalWeight += std::max(0, definition->randomWeight);
    }
    if (totalWeight <= 0) {
        return;
    }
    const uint32_t pick = Utils::hashSeedMix(*bindings.worldSeed, static_cast<int32_t>(tickCount), 0x524E444D) % static_cast<uint32_t>(totalWeight);
    int32_t cumulative = 0;
    for (int32_t definitionIndex = 0; definitionIndex < definitionCount; ++definitionIndex) {
        const WorldEventDefinition* definition = getWorldEventDefinition(definitionIndex);
        if (definition == nullptr || definition->triggerKind != WorldEventTriggerKind::Random) {
            continue;
        }
        cumulative += std::max(0, definition->randomWeight);
        if (pick < static_cast<uint32_t>(cumulative)) {
            tryFireDefinition(definitionIndex, tickCount);
            return;
        }
    }
}

void WorldEventSystem::updateHousingEconomy() {
    if (bindings.playerOperationsStore == nullptr || bindings.boroughVitalityStore == nullptr || bindings.chunkStore == nullptr) {
        return;
    }
    updatePlayerHousingRentMultiplier(*bindings.playerOperationsStore, *bindings.boroughVitalityStore, *bindings.chunkStore);
    if (bindings.worldEventStore != nullptr && isWorldEventFlagSet(*bindings.worldEventStore, WorldEventFlag::EconomicDistress)) {
        bindings.playerOperationsStore->rentEventAdjustmentBps = std::min(bindings.playerOperationsStore->rentEventAdjustmentBps, -400);
    }
}

bool WorldEventSystem::tryFireWorldEventById(const char* eventId, uint64_t tickCount) {
    const int32_t definitionIndex = findWorldEventDefinitionIndexById(eventId);
    if (definitionIndex < 0) {
        return false;
    }
    tryFireDefinition(definitionIndex, tickCount);
    return true;
}

void WorldEventSystem::onTick(uint64_t tickCount) {
    lastTickCount = tickCount;
    if (!areWatchesInitialized) {
        initializeWatches();
    }
    updateHousingEconomy();
    if (bindings.worldEventStore == nullptr) {
        return;
    }
    WorldEventStore& store = *bindings.worldEventStore;
    if (tickCount - store.lastConditionScanTick >= static_cast<uint64_t>(WORLD_EVENT_CONDITION_SCAN_INTERVAL_TICKS)) {
        store.lastConditionScanTick = tickCount;
        scanWatchEvents(tickCount);
        scanConditionEvents(tickCount);
    }
    scanRandomEvents(tickCount);
}

} // namespace Core
