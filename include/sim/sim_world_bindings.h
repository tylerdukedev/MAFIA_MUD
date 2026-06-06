#pragma once

#include "character/player_profile.h"
#include "game/criminal_record.h"
#include "game/evidence_system.h"
#include "game/investigation_case_store.h"
#include "game/player_criminal_justice.h"
#include "game/player_law_enforcement.h"
#include "game/player_world_state.h"
#include "game/police_contacts.h"
#include "game/player_organization.h"
#include "game/player_operations.h"
#include "game/player_wallet.h"
#include "game/street_crime.h"
#include "sim/character_agent.h"
#include "sim/agent_relationship_graph.h"
#include "sim/sim_event_queue.h"
#include "sim/world_event_store.h"
#include "world/chunk_store.h"
#include "world/city_control.h"
#include "world/tile_vitality.h"
#include "world/world_config.h"
#include <cstdint>

namespace Core {

struct SimWorldBindings {
    ChunkStore* chunkStore = nullptr;
    BoroughVitalityStore* boroughVitalityStore = nullptr;
    const WorldConfig* worldConfig = nullptr;
    uint64_t* worldSeed = nullptr;
    PlayerWallet* playerWallet = nullptr;
    CityControlStore* cityControlStore = nullptr;
    SimEventQueue* eventQueue = nullptr;
    PlayerProfile* playerProfile = nullptr;
    PlayerOperationsStore* playerOperationsStore = nullptr;
    PlayerOrganizationStore* playerOrganizationStore = nullptr;
    PlayerLawEnforcementStore* playerLawEnforcementStore = nullptr;
    PlayerCriminalJusticeStore* playerCriminalJusticeStore = nullptr;
    PlayerStreetCrimeStore* playerStreetCrimeStore = nullptr;
    CharacterAgentStore* characterAgentStore = nullptr;
    AgentRelationshipGraph* agentRelationshipGraph = nullptr;
    WorldEventStore* worldEventStore = nullptr;
    CriminalRecordStore* playerCriminalRecordStore = nullptr;
    PoliceContactStore* playerPoliceContactStore = nullptr;
    PlayerWorldState* playerWorldState = nullptr;
    InvestigationCaseStore* investigationCaseStore = nullptr;
    EvidenceSystemStore* evidenceSystemStore = nullptr;
};

bool isSimWorldBindingsValid(const SimWorldBindings& bindings);

} // namespace Core
