#pragma once

#include "character/character_draft.h"
#include "core/sim_clock.h"
#include "game/player_criminal_justice.h"
#include "game/player_law_enforcement.h"
#include "game/player_organization.h"
#include "game/player_operations.h"
#include "world/business_node_table.h"
#include "game/player_wallet.h"
#include "persistence/save_gameplay_stores.h"
#include "game/street_crime.h"
#include "sim/character_agent.h"
#include "sim/world_event_store.h"
#include "ui/map_camera.h"
#include "world/chunk_store.h"
#include "world/city_control.h"
#include "world/tile_vitality.h"
#include "world/world_config.h"
#include <cstdint>
#include <vector>

namespace Core {

constexpr const char* DEFAULT_SAVE_FILENAME = "capitalvice_save.dat";
constexpr int32_t SAVE_GAME_TILE_COUNT = WorldConfig::WORLD_WIDTH_TILES * WorldConfig::WORLD_HEIGHT_TILES;

struct SaveGameSnapshot {
    uint64_t worldSeed = 0;
    CharacterDraft characterDraft{};
    uint64_t tickCount = 0;
    bool isPaused = true;
    double speedMultiplier = 1.0;
    double accumulatorSeconds = 0.0;
    MapCamera mapCamera{};
    int64_t cashCents = 0;
    int64_t lifetimeLegitCents = 0;
    int64_t lifetimeCrimeCents = 0;
    std::vector<uint8_t> cityOwnerIds;
    HeadquartersKind headquartersKind = HeadquartersKind::None;
    int32_t employedBusinessIndex = -1;
    int32_t activeOperationCount = 0;
    int32_t activeCatalogIndices[MAX_OPERATION_CATALOG_COUNT]{};
    int32_t familyOpinionPenalty = 0;
    uint64_t headquartersEstablishedTick = 0;
    uint64_t lastMonthlyLedgerTick = 0;
    uint64_t lastFamilyUpkeepTick = 0;
    uint8_t headquartersRegionId = 0;
    int8_t consecutiveUnpaidRentMonths = 0;
    int32_t rentMultiplierBps = 10000;
    int32_t rentEventAdjustmentBps = 0;
    WorldEventStore worldEventStore{};
    CharacterAgentStore characterAgentStore{};
    PlayerOrganizationStore organizationStore{};
    PlayerLawEnforcementStore lawEnforcementStore{};
    PlayerStreetCrimeStore streetCrimeStore{};
    PlayerCriminalJusticeStore criminalJusticeStore{};
    int32_t workExperienceMonths = 0;
    uint64_t jobReapplyAvailableTickByBusiness[MAX_BUSINESS_NODE_COUNT]{};
    SaveGameplayStores gameplayStores{};
    std::vector<uint8_t> regionIds;
    std::vector<uint8_t> terrainIds;
    std::vector<int16_t> elevations;
    std::vector<uint32_t> flags;
    std::vector<uint8_t> economicWeights;
    std::vector<uint16_t> populations;
    std::vector<uint8_t> crimePressures;
    std::vector<uint8_t> lawPressures;
    std::vector<uint8_t> businessVitalities;
    std::vector<uint8_t> playerInfluences;
    std::vector<uint8_t> oppositionInfluences;
};

bool saveFileExists(const char* filePath);
bool buildSaveSnapshot(
    SaveGameSnapshot& outSnapshot,
    uint64_t worldSeed,
    const CharacterDraft& characterDraft,
    const SimClock& simClock,
    const MapCamera& mapCamera,
    const ChunkStore& chunkStore,
    const BoroughVitalityStore& boroughVitalityStore,
    const PlayerWallet& playerWallet,
    const CityControlStore& cityControlStore,
    const PlayerOperationsStore& playerOperationsStore,
    const WorldEventStore& worldEventStore,
    const CharacterAgentStore& characterAgentStore,
    const PlayerOrganizationStore& organizationStore,
    const PlayerLawEnforcementStore& lawEnforcementStore,
    const PlayerStreetCrimeStore& streetCrimeStore,
    const PlayerCriminalJusticeStore& criminalJusticeStore,
    const SaveGameplayStores& gameplayStores,
    int32_t workExperienceMonths);
bool applySaveSnapshot(
    const SaveGameSnapshot& snapshot,
    uint64_t& outWorldSeed,
    CharacterDraft& outCharacterDraft,
    SimClock& simClock,
    MapCamera& mapCamera,
    ChunkStore& chunkStore,
    PlayerWallet& playerWallet,
    CityControlStore& cityControlStore,
    PlayerOperationsStore& playerOperationsStore,
    WorldEventStore& worldEventStore,
    CharacterAgentStore& characterAgentStore,
    PlayerOrganizationStore& organizationStore,
    PlayerLawEnforcementStore& lawEnforcementStore,
    PlayerStreetCrimeStore& streetCrimeStore,
    PlayerCriminalJusticeStore& criminalJusticeStore,
    SaveGameplayStores& gameplayStores,
    int32_t& workExperienceMonths);
bool saveGameToFile(const char* filePath, const SaveGameSnapshot& snapshot);
bool loadGameFromFile(const char* filePath, SaveGameSnapshot& outSnapshot);

} // namespace Core
