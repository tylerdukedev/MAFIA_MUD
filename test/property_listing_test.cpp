#include "game/property_listing_store.h"
#include "game/property_worldgen.h"
#include "game/game_calendar.h"
#include "procgen/world_generator.h"
#include "world/tile_vitality.h"
#include <catch2/catch_test_macros.hpp>

using namespace Core;

TEST_CASE("Property listing store adds and retrieves records", "[property_listing]") {
    PropertyListingStore store{};
    const int32_t listingIndex = addPropertyListing(
        store,
        120,
        140,
        static_cast<uint8_t>(RegionId::Manhattan),
        PropertyListingTier::Apartment,
        static_cast<uint8_t>(PropertyListingPerks::NearTransit),
        350000,
        2800,
        2,
        0);
    REQUIRE(listingIndex == 0);
    const PropertyListingRecord* record = getPropertyListingRecord(store, listingIndex);
    REQUIRE(record != nullptr);
    REQUIRE(record->tier == PropertyListingTier::Apartment);
    REQUIRE(record->rentCents == 2800);
    REQUIRE(record->onMarket);
}

TEST_CASE("Property worldgen avoids node collisions and fills boroughs", "[property_listing]") {
    WorldConfig worldConfig{};
    ChunkStore chunkStore{worldConfig};
    BoroughVitalityStore vitalityStore{};
    PropertyStore propertyStore{};
    PropertyListingStore listingStore{};
    GameCalendarStore calendarStore{};
    constexpr uint64_t inputSeed = 424242ULL;
    WorldGenerator worldGenerator;
    worldGenerator.generate(worldConfig, chunkStore, inputSeed);
    rollupBoroughVitality(worldConfig, chunkStore, vitalityStore);
    generatePropertyListingsForNewGame(
        listingStore,
        propertyStore,
        chunkStore,
        vitalityStore,
        calendarStore,
        inputSeed);
    REQUIRE(listingStore.listingCount > 0);
    REQUIRE(propertyStore.recordCount == listingStore.listingCount);
    constexpr int32_t minSeparationTiles = 3;
    constexpr int32_t minSeparationSquared = minSeparationTiles * minSeparationTiles;
    for (int32_t index = 0; index < listingStore.listingCount; ++index) {
        const PropertyListingRecord& listing = listingStore.records[index];
        REQUIRE(listing.askPriceCents > 0);
        REQUIRE(listing.rentCents > 0);
        for (int32_t otherIndex = index + 1; otherIndex < listingStore.listingCount; ++otherIndex) {
            const PropertyListingRecord& otherListing = listingStore.records[otherIndex];
            const int32_t deltaX = listing.tileX - otherListing.tileX;
            const int32_t deltaY = listing.tileY - otherListing.tileY;
            REQUIRE(deltaX * deltaX + deltaY * deltaY > minSeparationSquared);
        }
    }
}
