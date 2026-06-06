#pragma once

#include "character/player_profile.h"
#include "core/sim_clock.h"
#include "game/player_criminal_justice.h"
#include "game/player_law_enforcement.h"
#include "game/bank_loan.h"
#include "game/player_operations.h"
#include "game/player_organization.h"
#include "game/player_wallet.h"
#include "game/property_listing_store.h"
#include "game/property_store.h"
#include "world/tile_vitality.h"
#include "game/player_world_state.h"
#include "game/social_action_catalog.h"
#include "game/street_crime.h"
#include "persistence/save_gameplay_stores.h"
#include "sim/character_agent.h"
#include "sim/sim_event_queue.h"
#include "sim/world_event_store.h"
#include "ui/context_help.h"
#include "ui/panel_visibility.h"
#include "ui/viewport_state.h"
#include "world/chunk_store.h"
#include "world/world_config.h"
#include <cstdint>

namespace Core {

// Single per-frame UI bundle: one tickCount for eligibility/cooldowns and shared store refs.
struct GameUiFrameContext {
    uint64_t tickCount = 0;
    uint64_t worldSeed = 0;
    const WorldConfig& worldConfig;
    ChunkStore& chunkStore;
    PlayerOperationsStore& playerOperationsStore;
    PropertyListingStore& propertyListingStore;
    PropertyStore& propertyStore;
    BankLoanStore& bankLoanStore;
    const BoroughVitalityStore& boroughVitalityStore;
    PlayerOrganizationStore& playerOrganizationStore;
    PlayerStreetCrimeStore& playerStreetCrimeStore;
    PlayerSocialActionStore& playerSocialActionStore;
    PlayerLawEnforcementStore& playerLawEnforcementStore;
    PlayerCriminalJusticeStore& playerCriminalJusticeStore;
    PlayerWallet& playerWallet;
    PlayerWorldState& playerWorldState;
    CharacterAgentStore& characterAgentStore;
    const WorldEventStore& worldEventStore;
    SimEventQueue& simEventQueue;
    const PlayerProfile& playerProfile;
    SaveGameplayStores& gameplayStores;
    SimClock& simClock;
    const ViewportPickState& viewportPickState;
    GamePanelVisibility& panelVisibility;
    ContextHelpState& contextHelpState;
};

} // namespace Core
