# Social Interaction & Criminal Justice Expansion

## Overview
This document outlines the expansion of social systems and criminal justice mechanics to create a more immersive, relationship-driven simulation where both player and NPCs follow the same rules.

---

## 1. Criminal Record System (Rap Sheet)

### Current State
- Tracks custody phases, legal tiers, arrest counts
- Heat/evidence system with warrants
- Generic "arrested for tier X crime" approach
- No persistent charge history

### Required Changes

#### Data Structure
**File**: `include/game/criminal_record.h`
```cpp
enum class ChargeType : uint8_t {
    // Misdemeanors
    Assault = 0,
    PettyTheft = 1,
    Trespassing = 2,
    Vandalism = 3,
    Disorderly = 4,
    
    // Felonies
    AggravatedAssault = 5,
    Robbery = 6,
    Extortion = 7,
    Racketeering = 8,
    MurderSecondDegree = 9,
    MurderFirstDegree = 10,
    
    // Financial
    Embezzlement = 11,
    MoneyLaundering = 12,
    TaxEvasion = 13,
    WireFlaud = 14,
};

enum class ChargeOutcome : uint8_t {
    Pending = 0,
    Dismissed = 1,
    PleaBargain = 2,
    GuiltyVerdict = 3,
    Acquitted = 4,
};

struct CriminalCharge {
    ChargeType chargeType = ChargeType::Assault;
    ChargeOutcome outcome = ChargeOutcome::Pending;
    CrimeLegalTier tier = CrimeLegalTier::PettyStreet;
    uint64_t arrestTick = 0;
    uint64_t resolutionTick = 0;
    int32_t sentenceTicks = 0;  // If convicted
    char arrestingOfficerName[32]{};
    char jurisdictionLabel[32]{};  // "Manhattan South Precinct", "NYPD Central"
};

constexpr int32_t MAX_CRIMINAL_CHARGES_PER_RECORD = 32;

struct CriminalRecordStore {
    CriminalCharge charges[MAX_CRIMINAL_CHARGES_PER_RECORD]{};
    int32_t chargeCount = 0;
    int32_t convictionCount = 0;
    int32_t priorFelonies = 0;
};
```

#### Integration Points
- When `beginPlayerArrest()` is called, also call `addCriminalCharge()`
- Charges persist in save game
- Court resolution updates charge outcome
- Rap sheet influences:
  - Bond amount (priors increase it)
  - Court outcome probability
  - Police attention/harassment
  - Job applications (employers check)
  - Rival respect/fear

---

## 2. Police Officer Relationship System

### Current State
- Generic "police" as faceless system
- Bribes reduce heat/warrants
- No individual officer tracking

### Required Changes

#### Data Structure
**File**: `include/game/police_contacts.h`
```cpp
constexpr int32_t MAX_POLICE_CONTACTS = 8;
constexpr int32_t OFFICER_OPINION_MIN = -100;
constexpr int32_t OFFICER_OPINION_MAX = 100;

enum class PoliceRank : uint8_t {
    Officer = 0,
    Detective = 1,
    Sergeant = 2,
    Lieutenant = 3,
};

struct PoliceContactState {
    bool isActive = false;
    char officerName[32]{};
    char badgeNumber[8]{};
    char precinct[32]{};  // "Midtown South", "Brooklyn North"
    PoliceRank rank = PoliceRank::Officer;
    int32_t opinionOfPlayer = 0;
    int32_t corruptibility = 50;  // 0=incorruptible, 100=dirty cop
    int32_t encounterCount = 0;
    int32_t bribeCount = 0;
    uint64_t lastEncounterTick = 0;
    bool isOnPayroll = false;  // Regular bribes
};

struct PoliceContactStore {
    PoliceContactState contacts[MAX_POLICE_CONTACTS]{};
};
```

#### Officer Behavior
**Friendly/Corrupt Officers** (opinion > 30, high corruptibility):
- May "forget" to file paperwork after arrest → charge dismissed
- Can tip player off about investigations
- Reduce evidence score when player commits crime in their precinct
- Accept bribes more easily

**Neutral Officers** (opinion -10 to 30):
- Follow procedure strictly
- No special treatment either way

**Hostile Officers** (opinion < -10):
- Increase witness chance when player operates nearby
- Push for maximum charges
- Testify strongly in court
- May "remember" player from prior encounters and harass

#### Creating Contacts
Officers become contacts through:
1. **First arrest** - arresting officer is remembered
2. **Repeated encounters** - same cop patrols your territory
3. **Bribe attempts** - even if failed, creates contact entry
4. **Introduction** - criminal contacts can introduce you to dirty cops

---

## 3. Enhanced Court System

### Current Courthouse UI
Currently shows bond hearing with generic tier.

### New Arraignment Screen
**File**: `src/ui/criminal_justice_ui.cpp`

When player can't afford bond, show full arraignment:

```
┌─────────────────────────────────────────────────────────────┐
│  CRIMINAL COURT OF THE CITY OF NEW YORK - MANHATTAN         │
│  100 Centre Street, New York, NY 10013                      │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  THE PEOPLE OF THE STATE OF NEW YORK                        │
│                  v.                                          │
│  DEFENDANT: [Player Name]                                   │
│  AGE: [Age]    ETHNICITY: [Italian/Irish/etc]               │
│                                                              │
│  DOCKET NO: [Generated]    DATE: [Calendar Date]            │
│                                                              │
├─────────────────────────────────────────────────────────────┤
│  CHARGES:                                                    │
│                                                              │
│  COUNT 1: Extortion (NYPL §155.40) - Class C Felony        │
│           Incident Date: [Date] | Precinct: Midtown South   │
│           Arresting Officer: Det. M. Russo #4429            │
│                                                              │
│  COUNT 2: Assault 2nd Degree (NYPL §120.05) - Class D Fel. │
│           Incident Date: [Date] | Precinct: Midtown South   │
│           Arresting Officer: Det. M. Russo #4429            │
│                                                              │
│  PRIOR RECORD: 2 prior arrests, 1 conviction               │
│                                                              │
├─────────────────────────────────────────────────────────────┤
│  BAIL: $15,000 cash or bond                                 │
│                                                              │
│  [Post Bail]  [Remain in Custody - Await Trial]            │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

#### Jurisdiction Labels
Realistic court names based on location:
- Manhattan: "Criminal Court of the City of New York - Manhattan"
- Brooklyn: "Kings County Criminal Court"
- Queens: "Queens County Criminal Court"
- Bronx: "Bronx County Criminal Court"
- Staten Island: "Richmond County Criminal Court"
- New Jersey: "Superior Court of New Jersey - [County]"

---

## 4. Social Interaction System

### Core Concept
A modal-based interaction system where player can initiate social actions with any character agent. NPCs use the same system for emergent behavior.

### Data Structure
**File**: `include/game/social_interaction.h`

```cpp
enum class SocialInteractionKind : uint8_t {
    Invite = 0,           // Invite to location
    Negotiate = 1,        // End rivalry, form alliance
    Intimidate = 2,       // Threaten into compliance
    Charm = 3,            // Build relationship
    ProposeWork = 4,      // Pitch a job together
    AskFavor = 5,         // Request help
    ShareIntel = 6,       // Trade information
};

enum class InteractionLocation : uint8_t {
    Bar = 0,
    Restaurant = 1,
    Park = 2,
    PrivateHome = 3,
    BackAlley = 4,
};

struct SocialInteractionOffer {
    SocialInteractionKind kind;
    const char* label;
    const char* description;
    int32_t relationshipRequirement;  // Min opinion needed
    bool requiresTravel;
    InteractionLocation suggestedLocation;
};
```

### Interaction Flow

1. **Player clicks on character contact**
2. **UI shows available interactions** based on:
   - Current relationship level
   - Character personality
   - Player's organization tier
   - Location/context

3. **If requires location (Invite)**:
   - Show location picker tied to travel system
   - Calculate travel time/cost
   - Both characters travel to location
   - Interaction happens at destination

4. **Interaction Resolution**:
   - Success chance based on:
     - Opinion/trust
     - Player charisma (from profile)
     - Character personality traits
     - Prior interactions
   - Update relationship stats
   - Potential outcomes logged

### Example: End Rivalry
**Requirements**:
- Target must be marked as rival
- Opinion must be > -30 (not deeply hostile)
- Player must offer something (territory, cash, favor)

**Resolution**:
```cpp
bool tryNegotiateEndRivalry(
    CharacterAgentState& rivalState,
    PlayerOrganizationStore& orgStore,
    CityControlStore& cityControl,
    int32_t offerType,  // 0=territory, 1=cash, 2=favor
    uint64_t worldSeed) {
    
    int32_t baseChance = 20;
    baseChance += rivalState.opinionOfPlayer / 2;
    baseChance += rivalState.respect / 3;
    
    if (offerType == 0) baseChance += 25;  // Territory is compelling
    if (offerType == 1) baseChance += 15;  // Cash is good
    if (offerType == 2) baseChance += 10;  // Favor is risky for them
    
    // Proud personality less likely to negotiate
    if (rivalState.generatedTrait == AgentPersonalityTrait::Proud) {
        baseChance -= 20;
    }
    
    uint32_t roll = (worldSeed % 100);
    return roll < static_cast<uint32_t>(baseChance);
}
```

### NPC Use of Social System
NPCs can initiate interactions:
- **Rival invites player** to "negotiate" → actually an ambush
- **Criminal contact proposes work** → recruitment for heist
- **Police officer invites to bar** → shakedown attempt
- **Family member asks favor** → family mission

---

## 5. Implementation Priority

### Phase 1: Criminal Record Foundation
1. Create `CriminalRecordStore` data structure
2. Add to save game serialization
3. Update arrest flow to create charges
4. Display charges in existing court modal

### Phase 2: Enhanced Court UI
1. Build arraignment screen layout
2. Generate jurisdiction labels from arrest location
3. Show full charge list with details
4. Add "Remain in Custody" flow for broke players

### Phase 3: Police Relationships
1. Create `PoliceContactStore`
2. Generate officers on first arrest
3. Track repeat encounters
4. Implement opinion-based behavior modifiers

### Phase 4: Social Interactions
1. Design interaction modal UI
2. Implement "End Rivalry" negotiation
3. Add "Invite to Location" with travel integration
4. Build NPC AI to use interaction system

---

## 6. Design Notes

### Realism vs Gameplay
- Use realistic court names and structure
- Charge types map to actual NY Penal Law (simplified)
- Officers have badge numbers for authenticity
- Balance: not every crime needs 10 charges, cluster related offenses

### Symmetry
**Critical**: NPCs must use same systems:
- NPC criminal records affect their behavior
- NPC police relationships influence their operations
- NPCs can negotiate with each other
- Creates emergent storytelling

### Performance
- Criminal records capped at 32 charges per character
- Police contacts capped at 8 officers
- Social interactions are modal (pauses sim) to avoid per-frame cost

---

## 7. Future Extensions

- **Witness testimony system**: Witnesses can be intimidated/bribed
- **Lawyer quality affects outcomes**: Better lawyers get charges dismissed
- **Three strikes system**: Multiple felonies trigger mandatory sentencing
- **Federal attention**: High-tier crimes attract FBI (separate from NYPD)
- **Courtroom drama**: Actual trial proceedings for major charges
