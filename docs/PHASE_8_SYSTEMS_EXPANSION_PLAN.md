# Phase 8 — Systems Expansion Plan

**Baseline:** Phases 0–7 in [`DEVELOPMENT_PLAN.md`](DEVELOPMENT_PLAN.md) are complete.

**User decisions:**
- **Era / pricing:** In-game calendar years advance; vehicle, property, loan, and cost curves scale with **economy simulation** (borough vitality + tile rollup).
- **Custody waits:** 1–3 minute **real-time** minimums advance **only while sim is unpaused**.

---

## Architecture principles

- [x] No heap in `runTick()` — pre-allocate at init/worldgen
- [x] Flat `uint32_t` IDs into SoA stores
- [x] UI reads up — no ImGui in sim layers
- [x] Shared road travel core for player and NPCs

---

## Recommended build order

| Order | Track | Status |
|-------|--------|--------|
| 1 | 8A — Shared travel + mobility | Complete |
| 2 | 8B — Map node taxonomy | Complete |
| 3 | 8D — Property / RE / banks / dealerships | Complete |
| 4 | 8C — Social actions | Complete |
| 5 | 8E — Justice / evidence | Complete |

---

## Track 8A — NPC traversal parity + mobility assets

- [x] `SharedTravelPath` + `shared_travel.cpp` extracted from player travel
- [x] NPC decisions use `findRoadPath` + `buildTravelPlan` + path tick
- [x] `MobilityAsset` roll on agent init (motive, cash, region, year)
- [x] Calendar ticks NPC travel every sim tick
- [x] `shared_travel_test.cpp` + extend `calendar_travel_test.cpp`

**Key files:** `shared_travel_state.h`, `shared_travel.cpp`, `npc_decision.cpp`, `calendar_system.cpp`, `npc_spatial_init.cpp`

---

## Track 8B — Location node visual taxonomy

- [x] `LandmarkKind` on landmark table
- [x] `map_marker_style.h` — unified circle outline + label zoom (4.5 px/tile)
- [x] Color by `LocationCategory` (landmark, industry, law, RE, bank, dealership, listing)
- [x] Refactor landmark/business/player/NPC renderers

---

## Track 8C — Social relationship actions

- [x] `social_action_catalog` — Visit, ShareMeal, SmallGift, Favor, Apologize, IntroduceContact, AskIntel, Negotiate, Warn
- [x] Contacts panel tabs: Build rapport / Pressure / Crew
- [x] Help manual topics
- [x] `social_action_test.cpp`

---

## Track 8D — Housing → property system

- [x] `PropertyListingStore` SoA (tile, tier, perks, ask/rent, on-market)
- [x] `property_worldgen` pass with collision avoidance
- [x] Real estate office + bank business node kinds
- [x] `bank_loan` — mortgage/auto APR from economy index
- [x] `homePropertyIndex` + `housingTenure` on `PlayerOperationsStore`
- [x] Property browser UI / RE office flow (`real_estate_ui.cpp`)
- [x] `property_listing_test.cpp`

---

## Track 8A/8D — Car dealerships

- [x] `BusinessNodeKind::CarDealershipNew` / `CarDealershipUsed`
- [x] Era+economy vehicle pricing (`vehicle_dealership.cpp`)
- [x] Cash or auto loan purchase
- [x] Employer + map node styling

---

## Track 8E — Criminal justice, evidence, pacing

- [x] `InvestigationCase` + `EvidenceItem` + `CrimeScene` stores
- [x] Crime scene on street crime (execution skill %)
- [x] Police dispatch + hidden witnesses + certainty → warrant at 90%
- [x] `CustodyTimer` — 60–180s real time, sim-unpaused only
- [x] Plea conference modal + trial system + `ny_penal_codes`
- [x] `evidenceScore` rollup for UI compatibility
- [x] `investigation_test.cpp`, `plea_trial_test.cpp`

---

## Cross-cutting: Economy index

- [x] `getBoroughEconomyMultiplier(RegionId, GameCalendarStore, BoroughVitalityStore)`
- [x] Wired to property, vehicles, loans, NPC mobility

---

## Save v16+

- [x] `PropertyListingStore`, loans, `homePropertyIndex`, agent travel/mobility fields
- [x] `InvestigationCaseStore` + `EvidenceSystemStore`
- [x] Migration from v15

---

## Deferred (documented)

- Full federal / RICO investigation flip (stub only)
- Consigliere betrayal simulation
- Racket upkeep/payroll empire loop
- Live police chase on map (dispatch notification only in v1)

---

## Acceptance gates

- [x] All new public utilities have Catch2 executables (106 tests pass)
- [x] Zero new allocations in `runTick()` (code review)
- [ ] Manual smoke: RE office → rent → crime → warrant → custody ≥1 min → plea → trial
