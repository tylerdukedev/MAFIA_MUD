#include "game/player_career.h"
#include "game/player_employment.h"
#include "world/business_node_table.h"
#include <cstdio>

namespace Core {

void formatPlayerCareerTitle(
    const PlayerProfile& profile,
    const PlayerOperationsStore& operationsStore,
    const PlayerOrganizationStore& organizationStore,
    char* outBuffer,
    int32_t bufferSize) {
    if (outBuffer == nullptr || bufferSize <= 0) {
        return;
    }
    if (organizationStore.powerTier == PlayerPowerTier::Organization) {
        std::snprintf(outBuffer, static_cast<size_t>(bufferSize), "Mob boss");
        return;
    }
    if (organizationStore.powerTier == PlayerPowerTier::Crew) {
        std::snprintf(outBuffer, static_cast<size_t>(bufferSize), "Crew captain");
        return;
    }
    if (organizationStore.crewMemberCount > 0) {
        std::snprintf(outBuffer, static_cast<size_t>(bufferSize), "Crew operator");
        return;
    }
    if (isPlayerEmployed(operationsStore)) {
        const BusinessNodeDefinition* business = getBusinessNodeDefinition(operationsStore.employedBusinessIndices[0]);
        if (business != nullptr) {
            std::snprintf(
                outBuffer,
                static_cast<size_t>(bufferSize),
                "%s at %s",
                businessIndustryToLabel(business->industry),
                business->mapLabel);
            return;
        }
        std::snprintf(outBuffer, static_cast<size_t>(bufferSize), "Employed");
        return;
    }
    (void)profile;
    std::snprintf(outBuffer, static_cast<size_t>(bufferSize), "Independent operator");
}

} // namespace Core
