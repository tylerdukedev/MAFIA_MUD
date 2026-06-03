#pragma once

#include "character/player_profile.h"
#include "game/player_operations.h"
#include "game/player_wallet.h"
#include "sim/character_agent.h"
#include <cstdint>

namespace Core {

bool tryEstablishFamilyFriendDpaHeadquarters(
    PlayerOperationsStore& store,
    PlayerWallet& wallet,
    const PlayerProfile& profile,
    CharacterAgentStore& agentStore,
    uint64_t tickCount);

void activateKinLandlordFromDpa(const PlayerProfile& profile, CharacterAgentStore& agentStore);

} // namespace Core
