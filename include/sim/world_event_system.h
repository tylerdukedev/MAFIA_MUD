#pragma once

#include "sim/isim_system.h"
#include "sim/sim_world_bindings.h"
#include "sim/world_event_store.h"

namespace Core {

class WorldEventSystem final : public ISimSystem {
public:
    void bind(const SimWorldBindings& bindings);
    const char* getName() const override;
    void onTick(uint64_t tickCount) override;
    void initializeWatches();
    bool tryFireWorldEventById(const char* eventId, uint64_t tickCount);

private:
    SimWorldBindings bindings{};
    bool areWatchesInitialized = false;
    bool evaluateCondition(const WorldEventDefinition& definition, uint64_t tickCount) const;
    void applyEffect(const WorldEventEffect& effect, uint64_t tickCount);
    void tryFireDefinition(int32_t definitionIndex, uint64_t tickCount);
    void scanConditionEvents(uint64_t tickCount);
    void scanWatchEvents(uint64_t tickCount);
    void scanRandomEvents(uint64_t tickCount);
    void updateHousingEconomy();
};

bool evaluateWorldEventCondition(
    const WorldEventCondition& condition,
    const WorldEventStore& eventStore,
    const PlayerOperationsStore& operationsStore,
    const PlayerWallet& wallet,
    const CharacterAgentStore& agentStore,
    const BoroughVitalityStore& boroughVitalityStore);

void applyWorldEventEffect(
    const WorldEventEffect& effect,
    WorldEventStore& eventStore,
    PlayerOperationsStore& operationsStore,
    PlayerWallet& wallet,
    CharacterAgentStore& agentStore,
    uint64_t tickCount);

} // namespace Core
