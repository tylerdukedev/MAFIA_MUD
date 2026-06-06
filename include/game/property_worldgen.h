#pragma once

#include "game/game_calendar.h"
#include "game/property_listing_store.h"
#include "game/property_store.h"
#include "world/chunk_store.h"
#include "world/tile_vitality.h"
#include <cstdint>

namespace Core {

void generatePropertyListingsForNewGame(
    PropertyListingStore& listingStore,
    PropertyStore& propertyStore,
    const ChunkStore& chunkStore,
    const BoroughVitalityStore& boroughVitalityStore,
    const GameCalendarStore& calendarStore,
    uint64_t worldSeed);

} // namespace Core
