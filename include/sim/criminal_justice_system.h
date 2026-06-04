#pragma once

#include "game/player_criminal_justice.h"
#include "game/player_information_feed.h"
#include "game/player_law_enforcement.h"
#include "sim/isim_system.h"
#include "sim/sim_world_bindings.h"

namespace Core {

class CriminalJusticeSystem final : public ISimSystem {
public:
    void bind(
        const SimWorldBindings& inputBindings,
        PlayerCriminalJusticeStore* inputJusticeStore,
        PlayerLawEnforcementStore* inputLawStore,
        PlayerInformationFeedStore* inputInformationFeedStore);
    const char* getName() const override;
    void onTick(uint64_t tickCount) override;

private:
    SimWorldBindings bindings{};
    PlayerCriminalJusticeStore* justiceStore = nullptr;
    PlayerLawEnforcementStore* lawStore = nullptr;
    PlayerInformationFeedStore* informationFeedStore = nullptr;
};

} // namespace Core
