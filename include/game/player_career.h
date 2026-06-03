#pragma once

#include "character/player_profile.h"
#include "game/player_operations.h"
#include "game/player_organization.h"

namespace Core {

void formatPlayerCareerTitle(
    const PlayerProfile& profile,
    const PlayerOperationsStore& operationsStore,
    const PlayerOrganizationStore& organizationStore,
    char* outBuffer,
    int32_t bufferSize);

} // namespace Core
