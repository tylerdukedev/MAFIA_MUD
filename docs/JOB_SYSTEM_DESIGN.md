# Job System Enhancement Design

## Current State
- Player can hold exactly one job at a time
- `PlayerOperationsStore.employedBusinessIndex` stores single job (int32_t)
- No distinction between full-time and part-time work
- Quitting is instant with no relationship consequences

## Required Changes

### 1. Add Job Schedule Types
**File**: `include/world/business_node_table.h`
```cpp
enum class JobScheduleType : uint8_t {
    FullTime = 0,
    PartTime = 1,
};
```

Add to `BusinessNodeDefinition`:
```cpp
JobScheduleType scheduleType = JobScheduleType::FullTime;
```

### 2. Update Business Catalog
**File**: `src/world/business_node_table.cpp`

Mark appropriate businesses as part-time:
- Low-wage retail (< 300 cents): PartTime
- Night shift only: PartTime
- High turnover: PartTime
- Everything else: FullTime

### 3. Multi-Job Support
**File**: `include/game/player_operations.h`

Replace:
```cpp
int32_t employedBusinessIndex = -1;
```

With:
```cpp
int32_t employedBusinessIndices[2] = {-1, -1};  // [0]=primary, [1]=secondary
```

**File**: `src/game/player_employment.cpp`

Add functions:
- `getPlayerJobCount()` - returns 0, 1, or 2
- `hasFullTimeJob()` - checks if any job is full-time
- `canAcceptJob(scheduleType)` - validates job combination rules
- `getJobSlotForNewHire(scheduleType)` - returns which slot to use

### 4. Job Combination Rules
- **1 full-time + 0 part-time**: Allowed
- **1 full-time + 1 part-time**: Allowed
- **2 full-time**: NOT allowed
- **0 full-time + 1 part-time**: Allowed
- **0 full-time + 2 part-time**: Allowed

When accepting a new full-time job while having a full-time job:
→ Trigger resignation modal

### 5. Resignation Flow
**File**: `include/ui/game_modal_ui.h`

Add modal type:
```cpp
GameModalKind::JobResignation
```

Modal fields:
```cpp
int32_t resigningFromBusinessIndex;
int32_t acceptingAtBusinessIndex;
```

**File**: `src/ui/game_modal_ui.cpp`

Resignation modal presents two options:
1. **Give two-week notice**: 
   - Boss opinion +8, trust +5
   - Job change delayed by 2 ticks (represents 2 weeks)
   - Player continues current job during notice period
   
2. **Quit immediately**:
   - Boss opinion -12, trust -8
   - Instant job change
   - Boss contact remains but damaged relationship

### 6. UI Updates
**File**: `src/ui/game_ui_panels.cpp`

Jobs panel should show:
```
Current employment:
• Mulberry Street Wholesale (Full-time) — $3.25/shift
• [Empty slot - can accept 1 part-time job]
```

Or:
```
Current employment:
• Mulberry Street Wholesale (Full-time) — $3.25/shift
• Jamaica Public Market (Part-time) — $2.95/shift
```

**File**: `src/ui/game_ui.cpp` (map businesses)

When clicking Apply on a business:
- If player has no conflicting jobs → Show interview modal
- If player has full-time and accepting full-time → Show resignation modal first
- If player already at 2 jobs → Show error message

### 7. Income Calculation
**File**: `src/sim/economy_system.cpp`

`computeEmployedLegitIncomePerTickCents()` must sum both jobs:
```cpp
float total = 0.0f;
for (int i = 0; i < 2; ++i) {
    if (store.employedBusinessIndices[i] >= 0) {
        const BusinessNodeDefinition* biz = getBusinessNodeDefinition(store.employedBusinessIndices[i]);
        if (biz != nullptr) {
            float monthlyCents = computeBusinessMonthlyWageCents(*biz) * JOB_MONTHLY_WAGE_MULTIPLIER;
            // Part-time jobs earn 60% of full rate
            if (biz->scheduleType == JobScheduleType::PartTime) {
                monthlyCents *= 0.6f;
            }
            total += monthlyCents / MONTHLY_LEDGER_INTERVAL_TICKS;
        }
    }
}
return total;
```

### 8. Boss Contacts
**File**: `include/sim/character_agent.h`

Need two boss slots:
```cpp
constexpr int32_t BOSS_AGENT_SLOT_INDEX = 2;
constexpr int32_t BOSS_SECONDARY_AGENT_SLOT_INDEX = 6;  // New
```

Or: Keep one boss slot and switch based on which job the player interacts with.
Decision: Keep single boss slot, switch when player quits/changes jobs.

### 9. Work Schedule Integration
**File**: `include/game/player_work_schedule.h`

Currently tracks one shift. With two jobs, need to handle:
- Two shift schedules
- Conflict detection (can't work two jobs at same time)
- Travel time between jobs

**Simplification for Phase 1**: 
- Assume shifts don't overlap (validate at hire time)
- Track only primary job's shift in work schedule
- Secondary job is "off-hours" and doesn't trigger commute modal

## Implementation Order
1. Add `JobScheduleType` enum and mark businesses as FT/PT
2. Change `employedBusinessIndex` to `employedBusinessIndices[2]`
3. Update all references to use array (save/load, employment functions, income calculation)
4. Add job combination validation logic
5. Implement resignation modal
6. Update UI to show both jobs
7. Test edge cases (quit during notice period, save/load with 2 jobs, etc.)

## Testing Checklist
- [ ] Can accept 1 full-time job
- [ ] Can accept 1 full-time + 1 part-time
- [ ] Can accept 2 part-time jobs
- [ ] Cannot accept 2 full-time jobs without resignation flow
- [ ] Two-week notice increases boss opinion
- [ ] Immediate quit decreases boss opinion
- [ ] Income calculation sums both jobs correctly
- [ ] Part-time jobs earn 60% of full-time rate
- [ ] Save/load preserves both jobs
- [ ] Quitting one job keeps the other active
- [ ] Boss contact switches when primary job changes
