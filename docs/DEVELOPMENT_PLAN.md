# Development Plan — CapitalVice

Dwarf Fortress-style systems simulation set in NYC (five boroughs) + parts of NJ.
Procedural map, modern minimal ImGui, strict performance foundations for scale.

**Rule for agents and contributors**: Work phases in order. Do not start a later phase until the current phase deliverables are checked off and building cleanly.

---

## Phase 0 — Bootstrap ✅ (current)

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

## Phase 1 — World Skeleton ✅ (current)

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

## Phase 2 — Sim Loop + Debug UI

**Goal**: Time advances; world is inspectable.

| Deliverable | Status |
|-------------|--------|
| `SimClock` integrated into main loop (fixed tick, pause, step, speed) | ⬜ |
| `SystemRegistry` — ordered no-op systems | ⬜ |
| ImGui: sim controls, cursor tile info, memory/chunk stats | ⬜ |

**Done when**: Tick counter increments; tile under cursor shows coord + region.

---

## Phase 3 — Procgen v1 (Geography-First)

**Goal**: Seed-driven map that roughly resembles NYC + NJ.

| Deliverable | Status |
|-------------|--------|
| Pass 1: land/water mask (coastline, Hudson, East River) | ⬜ |
| Pass 2: borough + NJ region assignment | ⬜ |
| Pass 3: terrain / elevation | ⬜ |
| Generate-on-start wired to `ChunkStore` | ⬜ |

**Done when**: New seed → recognizable geography with labeled boroughs in UI.

---

## Phase 4 — Map Renderer

**Goal**: Pan/zoom map viewport.

| Deliverable | Status |
|-------------|--------|
| Orthographic 2D camera (pan, zoom) | ⬜ |
| Tile rendering from chunk SoA (color by terrain/region) | ⬜ |
| Input routing (`WantCaptureMouse` vs map pan) | ⬜ |

**Done when**: Smooth navigation; click tile → inspector updates.

---

## Phase 5 — First Gameplay System

**Goal**: Prove a real system plugs into the pipeline cleanly.

Pick **one**: zone control, sparse population, or resource nodes.

| Deliverable | Status |
|-------------|--------|
| SoA entity/agent store (if needed) | ⬜ |
| One `SimSystem` implementation | ⬜ |
| UI reflects system state changes | ⬜ |

**Done when**: System modifies world state each tick; visible in UI.

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
