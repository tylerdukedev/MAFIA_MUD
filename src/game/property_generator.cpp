#include "game/property_generator.h"
#include "game/operation_types.h"
#include "game/property_store.h"
#include "world/chunk_store.h"
#include "world/region_table.h"
#include "world/world_config.h"

namespace Core {

void generateProperties(
    PropertyStore& propertyStore,
    const ChunkStore& chunkStore,
    uint64_t worldSeed) {

    resetPropertyStore(propertyStore);

    // Generate a distributed set of properties across NYC boroughs
    // Focus on Manhattan, Brooklyn, Queens, Bronx
    const int32_t PROPERTIES_PER_BOROUGH = 20;

    const RegionId targetRegions[] = {
        RegionId::Manhattan,
        RegionId::Brooklyn,
        RegionId::Queens,
        RegionId::Bronx,
    };

    uint64_t seed = worldSeed;

    for (const RegionId regionId : targetRegions) {
        const uint8_t regionIdValue = static_cast<uint8_t>(regionId);
        int32_t propertiesInRegion = 0;

        // Try to place properties in this borough
        for (int32_t attempt = 0; attempt < PROPERTIES_PER_BOROUGH * 3 && propertiesInRegion < PROPERTIES_PER_BOROUGH; ++attempt) {
            seed = seed * 1103515245 + 12345;
            const int32_t tileX = static_cast<int32_t>((seed % WorldConfig::WORLD_WIDTH_TILES));
            seed = seed * 1103515245 + 12345;
            const int32_t tileY = static_cast<int32_t>((seed % WorldConfig::WORLD_HEIGHT_TILES));

            const WorldCoord coord{tileX, tileY};
            const RegionId chunkRegion = chunkStore.getRegionAt(coord);
            if (chunkRegion != regionId) {
                continue;
            }

            // Determine property type based on seed
            seed = seed * 1103515245 + 12345;
            const uint32_t typeRoll = seed % 100;

            HeadquartersKind propertyKind;
            int32_t monthlyCost;

            if (typeRoll < 70) {
                // 70% are rented rooms (cheap)
                propertyKind = HeadquartersKind::RentedRoom;
                monthlyCost = HQ_RENTED_ROOM_COST_CENTS;
            } else {
                // 30% are apartments (moderate)
                propertyKind = HeadquartersKind::Apartment;
                monthlyCost = HQ_APARTMENT_COST_CENTS;
            }

            addPropertyRecord(propertyStore, tileX, tileY, propertyKind, monthlyCost, regionIdValue);
            propertiesInRegion += 1;
        }
    }
}

} // namespace Core
