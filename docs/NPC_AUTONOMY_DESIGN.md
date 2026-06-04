# NPC Autonomy & Justice System Integration

## Core Principle
**ALL systems apply equally to player and NPCs.** NPCs are not scripted actors but autonomous agents with their own lives, making decisions based on motives, needs, traits, and relationships.

---

## 1. NPC Foundational Layer

### Spatial Presence
Every character agent exists on the map with position and state.

**File**: `include/sim/character_agent.h` (extend existing)
```cpp
struct CharacterAgentState {
    // Existing fields...
    int32_t opinionOfPlayer = 0;
    int32_t trust = 50;
    // ... etc
    
    // NEW: Spatial & Life Simulation
    int32_t currentTileX = -1;
    int32_t currentTileY = -1;
    bool isVisibleOnMap = false;  // Hidden when: at home, at work, in building, traveling abroad, dead
    uint8_t currentActivity = 0;  // enum: AtHome, AtWork, Traveling, InBuilding, Abroad, Incarcerated
    int32_t homePropertyIndex = -1;  // Links to property system
    int32_t workplaceBusinessIndex = -1;
    int32_t cashCents = 0;  // NPC wallet
    uint64_t activityStartTick = 0;
    int32_t destinationTileX = -1;
    int32_t destinationTileY = -1;
};
```

### Spawn Behavior
At game start, all character agents:
1. **Spawn at random location** (or their home if they have one)
2. **Assigned random home** from available properties (foundational property system)
3. **Assigned job** if employed (mirrors player employment)
4. **Given starting cash** based on job/background
5. **Rendered on map** with color + black outline (same as player marker)

### Map Visibility Rules
Character disappears from map when:
- **At home** (not traveling)
- **At work** (during work hours)
- **Inside building** (entered a business/location)
- **Traveling abroad** (trip outside NYC)
- **Dead** (permanent removal)
- **Incarcerated** (in jail/prison)

Character visible on map when:
- **Traveling** between locations (adheres to travel system)
- **Idle** in public space (park, street corner)
- **Meeting someone** at public location

---

## 2. Property System (Foundational)

### Minimal Viable Property System
**File**: `include/game/property_system.h`

```cpp
enum class PropertyType : uint8_t {
    Apartment = 0,
    House = 1,
    Business = 2,
};

enum class PropertyOwnership : uint8_t {
    Rented = 0,
    Owned = 1,
    Mortgaged = 2,
};

struct PropertyRecord {
    PropertyType type = PropertyType::Apartment;
    PropertyOwnership ownership = PropertyOwnership::Rented;
    int32_t tileX = -1;
    int32_t tileY = -1;
    int32_t monthlyPaymentCents = 0;  // Rent or mortgage
    int32_t ownerAgentIndex = -1;  // -1 = unowned, 0+ = agent index
    bool isPlayerOwned = false;
};

constexpr int32_t MAX_PROPERTIES = 256;

struct PropertyStore {
    PropertyRecord properties[MAX_PROPERTIES]{};
    int32_t propertyCount = 0;
};
```

**Integration**: At world generation, create property records at housing locations. Assign some to NPCs at spawn.

---

## 3. NPC Decision Making (Motive System)

### Motive Evaluation
NPCs already have `AgentMotive primaryMotive` (Survival, Wealth, Status, Loyalty, Revenge). Expand this to drive behavior.

**File**: `include/sim/character_decision.h` (new)

```cpp
enum class NPCNeed : uint8_t {
    Money = 0,        // Low cash, needs income
    Safety = 1,       // High danger, avoid threats
    Respect = 2,      // Low status, wants recognition
    Revenge = 3,      // Wronged, wants payback
    Loyalty = 4,      // Protect allies/family
    Territory = 5,    // Wants to control area
};

struct NPCDecisionContext {
    const CharacterAgentState* agentState;
    const PlayerOperationsStore* playerOpsStore;
    const PlayerOrganizationStore* playerOrgStore;
    const CityControlStore* cityControl;
    int32_t agentIndex;
    uint64_t tickCount;
    uint64_t worldSeed;
};

// Weights needs based on motive, traits, current state
int32_t evaluateNeedPriority(NPCNeed need, const NPCDecisionContext& context);

// Decides what action NPC takes this tick
void tickNPCDecisionMaking(CharacterAgentStore& agentStore, const NPCDecisionContext& context);
```

### Decision Flow
Each NPC tick (every ~20-40 ticks to reduce CPU):
1. **Evaluate needs**: Money, Safety, Respect, etc.
2. **Weight by motive**: Wealth motive → Money need is 2x priority
3. **Check current state**: Employed? Cash low? Under threat?
4. **Choose action**: Go to work, commit crime, travel home, meet contact, flee danger
5. **Execute**: Update position, trigger travel system, apply crime roll

---

## 4. Enhanced Criminal Justice Integration

### Existing Systems (Do Not Duplicate)
- **Legal Counsel**: `include/game/legal_counsel.h` already exists
- **Travel System**: `include/game/travel_modes.h` already exists
- **Criminal Justice**: `include/game/player_criminal_justice.h` exists but player-only

### Required Changes

#### A. Criminal Record System
**File**: `include/game/criminal_record.h` (new, integrates with existing justice)

```cpp
enum class ChargeType : uint8_t {
    Assault = 0, Robbery = 1, Extortion = 2, 
    Murder = 3, Racketeering = 4, Embezzlement = 5,
    // ... 14 total
};

struct CriminalCharge {
    ChargeType chargeType;
    uint8_t outcome;  // Pending, Dismissed, Convicted
    uint8_t tier;
    uint64_t arrestTick;
    char arrestingOfficer[32];
    char jurisdiction[32];  // "Manhattan South", "NYPD Central"
};

constexpr int32_t MAX_CHARGES_PER_RECORD = 16;

struct CriminalRecordStore {
    CriminalCharge charges[MAX_CHARGES_PER_RECORD];
    int32_t chargeCount = 0;
    int32_t convictionCount = 0;
};
```

**Integration**: 
- Add `CriminalRecordStore records[MAX_CHARACTER_AGENT_COUNT]` to `CharacterAgentStore`
- When `beginPlayerArrest()` called, also call `addCriminalCharge(recordStore, agentIndex, chargeType, tier)`
- NPCs arrested → same function, different agent index

#### B. Court Proceedings with Delays
**Existing**: Bond hearing modal shows immediately after arrest.

**New Flow** (when player can't afford bond):
1. Player chooses "Remain in Custody"
2. Modal closes
3. **Simulation UNPAUSES and ticks forward** (2-6 weeks simulated)
4. **Random jail events** fire periodically (fights, negotiations, intel gathering)
5. **Court date arrives** → new modal shows arraignment screen
6. Resolution depends on:
   - Lawyer quality (existing legal counsel system)
   - Prosecutor (new NPC agent with career)
   - Judge (new NPC agent with career/bias)
   - Evidence/witnesses (existing law enforcement system)

**File**: `include/game/court_proceeding.h` (new)

```cpp
struct CourtProceeding {
    int32_t prosecutorAgentIndex = -1;
    int32_t judgeAgentIndex = -1;
    int32_t defenseAttorneyTier = 0;  // Links to legal_counsel.h
    uint64_t courtDateTick = 0;
    int32_t delayWeeks = 0;  // 2-6 weeks typical
    bool isActive = false;
};

// Rolls prosecutor/judge, sets court date
void beginCourtProceeding(
    CourtProceeding& proceeding,
    CharacterAgentStore& agentStore,
    uint64_t currentTick,
    uint64_t worldSeed);

// Determines outcome based on all factors
CourtOutcome resolveCourtProceeding(
    const CourtProceeding& proceeding,
    const CriminalRecordStore& record,
    const CharacterAgentStore& agentStore,
    const PlayerLawEnforcementStore& lawStore,
    uint64_t worldSeed);
```

#### C. Jail Event System
**File**: `include/game/jail_events.h` (new)

```cpp
enum class JailEventType : uint8_t {
    Fight = 0,
    IntelGathered = 1,
    AllyMade = 2,
    Shakedown = 3,
    Message = 4,  // News from outside
};

struct JailEvent {
    JailEventType type;
    char description[128];
    int32_t opinionDelta = 0;  // If involves contact
    int32_t healthDelta = 0;
    int32_t intelValue = 0;
};

// Fires random event while player is in custody
bool tryFireJailEvent(
    JailEvent& outEvent,
    const PlayerCriminalJusticeStore& justiceStore,
    CharacterAgentStore& agentStore,
    uint64_t worldSeed,
    uint64_t tickCount);
```

**Integration**: In `tickPlayerCriminalJustice()`, when `custodyPhase == InJail`, periodically call `tryFireJailEvent()` and push to information feed.

#### D. Realistic Court Jurisdiction
**File**: `src/game/court_jurisdiction.cpp` (new)

```cpp
const char* getCourtNameForLocation(int32_t tileX, int32_t tileY, const ChunkStore& chunkStore) {
    const RegionId region = getRegionAtTile(chunkStore, tileX, tileY);
    switch (region) {
        case RegionId::Manhattan: return "Criminal Court of the City of New York - Manhattan";
        case RegionId::Brooklyn: return "Kings County Criminal Court";
        case RegionId::Queens: return "Queens County Criminal Court";
        case RegionId::Bronx: return "Bronx County Criminal Court";
        case RegionId::StatenIsland: return "Richmond County Criminal Court";
        default: return "Superior Court of New Jersey";
    }
}
```

---

## 5. Police Officer Contacts

### Integration with Existing Agent System
Police officers are character agents with special roles.

**Extend**: `include/sim/character_agent.h`

```cpp
enum class AgentRole : uint8_t {
    None = 0,
    Friend = 1,
    Family = 2,
    Criminal = 3,
    Boss = 4,
    Landlord = 5,
    PoliceOfficer = 6,  // NEW
    Prosecutor = 7,      // NEW
    Judge = 8,           // NEW
};

// Add to CharacterAgentState
struct CharacterAgentState {
    // ... existing fields
    AgentRole role = AgentRole::None;
    char badgeNumber[8]{};  // If police
    char precinct[32]{};    // If police
    int32_t corruptibility = 50;  // 0-100, if police
};
```

**Integration**:
- On first arrest, call `spawnArrestingOfficer(agentStore, arrestLocation)` → creates officer in empty agent slot
- Officer opinion affects: evidence tampering, charge dismissal, tip-offs
- Hostile officers increase witness chance in their precinct

---

## 6. Social Interaction System

### Tie to Existing Travel System
When player invites NPC to location:
1. **Calculate travel time** using existing `computeTravelTimeTicks()` from `travel_modes.h`
2. **Both characters travel** (player + NPC update positions over time)
3. **Arrive at location** → trigger interaction modal
4. **Interaction resolves** → both return home or continue activities

**File**: `include/game/social_interaction.h` (new)

```cpp
enum class InteractionKind : uint8_t {
    Negotiate = 0,  // End rivalry, form alliance
    Intimidate = 1,
    Invite = 2,     // Meet at location
};

struct SocialInteraction {
    InteractionKind kind;
    int32_t targetAgentIndex = -1;
    int32_t meetingLocationX = -1;
    int32_t meetingLocationY = -1;
    bool playerTravelingToMeeting = false;
    bool targetTravelingToMeeting = false;
    uint64_t scheduledMeetingTick = 0;
};

// Returns true if relationship allows this interaction
bool canInitiateInteraction(
    InteractionKind kind,
    const CharacterAgentState& targetAgent);

// Schedules meeting, starts travel for both parties
void beginSocialInteraction(
    SocialInteraction& interaction,
    InteractionKind kind,
    int32_t targetAgentIndex,
    int32_t locationX,
    int32_t locationY,
    CharacterAgentStore& agentStore,
    uint64_t currentTick);
```

---

## 7. Implementation Phases

### Phase 1: Foundational NPC Life Sim (Week 1)
- [ ] Add position/activity fields to `CharacterAgentState`
- [ ] Create minimal `PropertyStore` with apartment records
- [ ] Spawn NPCs with homes, jobs, cash at game start
- [ ] Render NPCs on map with visibility rules
- [ ] NPCs travel home/work using existing travel system

### Phase 2: Criminal Record & Court Delays (Week 2)
- [ ] Create `CriminalRecordStore` with charge tracking
- [ ] Integrate with arrest flow
- [ ] Add court proceeding delays (simulation ticks while in jail)
- [ ] Implement jail event system
- [ ] Create arraignment UI showing charges

### Phase 3: Court NPCs & Proceedings (Week 3)
- [ ] Add prosecutor/judge agent roles
- [ ] Expand legal counsel integration
- [ ] Court outcome based on lawyer vs prosecutor
- [ ] Judge bias/career affects sentencing
- [ ] Heritage/ethnicity influences (realism)

### Phase 4: Police Relationships (Week 4)
- [ ] Add police officer role to agents
- [ ] Spawn arresting officer on first arrest
- [ ] Opinion-based behavior (corrupt vs hostile)
- [ ] Repeat encounters build relationships

### Phase 5: NPC Decision Making (Week 5)
- [ ] Implement motive-driven need evaluation
- [ ] NPCs choose actions based on needs
- [ ] NPCs commit crimes, get arrested, go to court
- [ ] NPCs form relationships with each other

### Phase 6: Social Interactions (Week 6)
- [ ] "End Rivalry" negotiation modal
- [ ] "Invite to Location" with travel integration
- [ ] NPCs can initiate interactions with player
- [ ] NPCs interact with each other (emergent)

---

## 8. Key Design Principles

### Symmetry
- Player gets arrested → same function as NPC arrest
- Player goes to court → same proceeding system as NPC court
- Player travels → same travel system as NPC travel
- **No separate "player" and "NPC" code paths**

### Performance
- NPC decision making: tick every 20-40 game ticks (not every frame)
- Only visible NPCs update position every tick
- Hidden NPCs (at home/work) skip spatial updates
- Use agent slots efficiently (32 max agents)

### Integration
- **Use existing systems**: Travel, legal counsel, criminal justice
- **Extend, don't replace**: Add fields to existing structs
- **Consistent APIs**: Same function signatures for player/NPC

### Emergent Gameplay
- Prosecutor has Wealth motive → accepts bribes easily
- Judge has Loyalty motive + Italian heritage → lenient on Italian defendants
- Cop has Survival motive + low pay → becomes corrupt over time
- Rival has Revenge motive → initiates ambush meeting

---

## 9. Testing Strategy

Each phase must include integration tests:
- **Phase 1**: NPC spawns with home, travels to work, disappears from map
- **Phase 2**: Player arrested → charge created → court delayed → jail event fires
- **Phase 3**: Court proceeding → prosecutor/judge rolled → outcome varies by lawyer
- **Phase 4**: Arrest creates officer contact → repeat arrest → relationship changes
- **Phase 5**: NPC low cash + Wealth motive → commits crime → gets arrested
- **Phase 6**: Player invites rival → both travel → negotiation → rivalry ends

No changes to unrelated files. No unnecessary reads.
