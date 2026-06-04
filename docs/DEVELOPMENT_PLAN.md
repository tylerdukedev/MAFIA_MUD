# Development Plan — CapitalVice

Dwarf Fortress-style systems simulation set in NYC (five boroughs) + parts of NJ.
Procedural map, modern minimal ImGui, strict performance foundations for scale.

**Rule for agents and contributors**: Work phases in order. Do not start a later phase until the current phase deliverables are checked off and building cleanly.

---

## Phase 0 — Bootstrap ✅

**Goal**: Runnable application with window, ImGui shell, and sim-clock skeleton.

| Deliverable | Status |
|-------------|--------|
| CMake project (C++17, GLFW via FetchContent, vendored ImGui) | ✅ |
| `Core::Application` — init / run / shutdown | ✅ |
| GLFW + OpenGL 3 + ImGui backends wired | ✅ |
| Minimal UI: FPS, pause/resume, tick counter | ✅ |
| `.cursorrules` + this plan document | ✅ |

**Done when**: `cmake --build build && ./build/capital_vice` opens a window with ImGui panels.

---

## Phase 1 — World Skeleton ✅

**Goal**: Empty NYC-scale world in memory; no gameplay yet.

| Deliverable | Status |
|-------------|--------|
| Core types: `TileId`, `ChunkId`, `RegionId`, `WorldCoord`, `ChunkCoord`, `LocalTileCoord` | ✅ |
| `WorldConfig` — dimensions, chunk size, derived counts | ✅ |
| `RegionTable` — five boroughs + NJ zone definitions | ✅ |
| `ChunkStore` — SoA tile arrays, coord conversion, bounds checks | ✅ |
| Unit tests: chunk indexing, coord conversion, boundary checks | ✅ |

**Done when**: Tests pass; app shows world dimension stats in debug UI.

---

## Phase 1.5 — Docking UI ✅

**Goal**: Unity-style dockable panels with persisted layout.

| Deliverable | Status |
|-------------|--------|
| ImGui docking branch enabled | ✅ |
| Full-window dockspace + default layout | ✅ |
| Panels: Simulation, Regions, Map Viewport, Tile Inspector | ✅ |
| Layout reset menu + `capitalvice_layout.ini` persistence | ✅ |
| Minimum window size 960×540 | ✅ |

---

## Phase 2 — Sim Loop + Debug UI ✅

**Goal**: Time advances; world is inspectable.

| Deliverable | Status |
|-------------|--------|
| `SimClock` integrated into main loop (fixed tick, pause, step, speed) | ✅ |
| `SystemRegistry` — ordered no-op systems | ✅ |
| ImGui: sim controls, cursor tile info, memory/chunk stats | ✅ |

**Done when**: Tick counter increments; tile under cursor shows coord + region.

---

## Phase 3 — Procgen v1 (Geography-First) ✅

**Goal**: Seed-driven map that roughly resembles NYC + NJ.

| Deliverable | Status |
|-------------|--------|
| Pass 1: land/water mask (coastline, Hudson, East River) | ✅ |
| Pass 2: borough + NJ region assignment | ✅ |
| Pass 3: terrain / elevation | ✅ |
| Generate-on-start wired to `ChunkStore` | ✅ |

**Done when**: New seed → recognizable geography with labeled boroughs in UI.

---

## Phase 4 — Map Renderer ✅

**Goal**: Pan/zoom map viewport.

| Deliverable | Status |
|-------------|--------|
| Orthographic 2D camera (pan, zoom) | ✅ |
| Tile rendering from chunk SoA (color by terrain/region) | ✅ |
| Input routing (`WantCaptureMouse` vs map pan) | ✅ |

**Done when**: Smooth navigation; click tile → inspector updates.

---

## Phase 5 — Borough Vitality System

**Goal**: Tile-level signals roll up to borough economic health, population, and crime metrics with a performant sim pass and UI.

| Deliverable | Status |
|-------------|--------|
| Tile vitality SoA in `ChunkStore` + `tile_vitality` module | ✅ |
| `BoroughVitalitySystem` + worldgen vitality pass | ✅ |
| Simulation, Boroughs, Tile Inspector, District panels + manual topics | ✅ |
| Save format v3 (vitality arrays) + `tile_vitality_test` | ✅ |

**Done when**: System modifies world state each tick; borough metrics visible in UI after rollup.

---

## Phase 6 — Economy, Cities & Starting Placement ✅

**Goal**: Cents-based money, starting borough → random city landmark, first city claim, and passive income — with save v4.

| Deliverable | Status |
|-------------|--------|
| `PlayerWallet` — starting cash roll ($0–$25 weighted), debit/credit, broke threshold | ✅ |
| Character creation — Starting Borough, roll city + cash preview | ✅ |
| `CityControlStore` + `SimEventQueue` + `CityControlSystem` (Establish operation) | ✅ |
| `EconomySystem` — legit/crime income accrual, starting influence at city | ✅ |
| District panel renamed to **City**; crime heat map overlay toggle | ✅ |
| Save format v4 (wallet + per-landmark owners) | ✅ |
| Tests: `player_wallet_test`, `city_control_test`, save round-trip | ✅ |

**Done when**: New game spawns at rolled city with starting cash; player can claim a city when affordable; income ticks in Character panel; save/load preserves wallet and city ownership.

---

## Phase 7 — Criminal Justice Expansion ✅

**Goal**: Persistent rap sheet, named arresting officers, and a realistic arraignment modal.

| Deliverable | Status |
|-------------|--------|
| `CriminalRecordStore` — `CriminalCharge`, `ChargeType`, `ChargeOutcome`, statute labels | ✅ |
| `PoliceContactStore` — `PoliceContactState`, `PoliceRank`, procedural officer generation | ✅ |
| `beginPlayerArrest` — generates officer, records charge, wires into both stores | ✅ |
| `resolvePlayerCourt` — resolves latest pending charge on court outcome | ✅ |
| BondHearing modal — full arraignment layout: charges, statute, officer, prior record | ✅ |
| `jail_events.h/.cpp` — Fight/Intel/Ally/Shakedown/Message events fire during custody → info feed | ✅ |
| Randomized pretrial detention (held-without-bond varies by `JUSTICE_PRETRIAL_DELAY_VARIANCE_TICKS`) | ✅ |
| Save format v15 — persists `CriminalRecordStore` + `PoliceContactStore` + `PropertyStore` | ✅ |
| NPC autonomy skeleton — `NpcAutonomySystem`, `tickNpcDecisions`, map markers | ✅ |
| Property system — `PropertyStore`, `PropertyGenerator`, NPC home assignment | ✅ |

**Done when**: Player arrested → specific charge generated with named officer and jurisdiction → BondHearing shows full arraignment → court resolves charge into record → all tests green.

---

## Architecture Reference

```
Platform (Application, GLFW)
    ↓
Presentation (MapRenderer, GameUi)
    ↓
World (ChunkStore, WorldConfig, RegionTable, Procgen)
    ↓
Simulation (SimClock, SystemRegistry, EventQueue)
    ↓
Gameplay (future systems)
```

## Key Constants (Phase 1 defaults)

| Constant | Value | Notes |
|----------|-------|-------|
| Chunk size | 32×32 tiles | Fixed per chunk |
| World size | 512×512 tiles | ~coarse metro scale; refine later |
| Sim tick rate | 20 Hz | Configurable via `SimClock` |
| Regions | 6 | 5 boroughs + NJ |

---

## Building (Linux)

```bash
CC=gcc CXX=g++ cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
ctest --test-dir build --output-on-failure
./build/capital_vice
```

Dependencies fetched automatically via CMake FetchContent (GLFW, Catch2). System packages: `build-essential`, `libgl1-mesa-dev`, X11 dev libs.
