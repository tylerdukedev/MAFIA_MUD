#pragma once

#include "sim/isim_system.h"
#include "sim/sim_world_bindings.h"
#include "sim/character_agent.h"
#include <cstdint>

namespace Core {

class OperationSystem : public ISimSystem {
public:
    void bind(const SimWorldBindings& inputBindings, CharacterAgentStore* inputAgentStore);
    const char* getName() const override;
    void onTick(uint64_t tickCount) override;

private:
    SimWorldBindings bindings{};
    CharacterAgentStore* agentStore = nullptr;
    void processEstablishOperationEvent(const SimEvent& event);
    void processApplyForJobEvent(const SimEvent& event);
};

} // namespace Core
