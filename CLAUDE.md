# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

**CapitalVice** — Dwarf Fortress-style systems simulation set in NYC (five boroughs) + parts of NJ. Raw C++17, CMake, GLFW, OpenGL 3, Dear ImGui. No game engine.

Before starting any work, read `docs/DEVELOPMENT_PLAN.md` to check phase status. Work phases in order — do not implement later-phase systems before current deliverables are checked off. Update phase checkboxes when completing deliverables.

## Build & Run

```bash
# Configure (first time fetches GLFW, Catch2, ImGui via FetchContent)
CC=gcc CXX=g++ cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build -j$(nproc)

# Run all tests
ctest --test-dir build --output-on-failure

# Run a single test binary
./build/chunk_store_test

# Run the game
./build/capital_vice
```

On Windows, use the PowerShell installer script for packaging:
```powershell
powershell -ExecutionPolicy Bypass -File scripts\build_installer.ps1
```

Dev console (optional ImGui viewport panel) is ON by default via `CAPITALVICE_DEV_CONSOLE`. Disable with `-DCAPITALVICE_DEV_CONSOLE=OFF`.

## Architecture

The five layers build bottom-up — never skip or reverse them:

```
Platform      Core::Application — GLFW window, OpenGL context, main loop
    ↓
Presentation  src/ui/ — MapRenderer, GameUi panels, ImGui overlay
    ↓
World         ChunkStore, WorldConfig, RegionTable, Procgen — SoA tile data
    ↓
Simulation    SimClock (20 Hz fixed tick), SystemRegistry, SimEventQueue
    ↓
Gameplay      game/ systems — economy, crime, employment, criminal justice
```

**Critical rule**: Simulation and gameplay code must never include ImGui headers. Data flows down; UI reads up.

### Key types

- `Core::Application` (`include/core/application.h`) — owns all stores as members; wires everything together. This is the composition root.
- `Core::SystemRegistry` (`include/sim/system_registry.h`) — holds 13 simulation systems and runs them in order each tick via `runTick(tickCount)`. Systems: `DebugSystem`, `StreetCrimeSystem`, `OperationSystem`, `CityControlSystem`, `WorldEventSystem`, `PoliceSystem`, `CriminalJusticeSystem`, `EconomySystem`, `BoroughVitalitySystem`, `CalendarSystem`.
- `ChunkStore` (`include/world/chunk_store.h`) — SoA tile arrays in fixed 32×32 chunks; 512×512 world. All tile access goes through coord conversion utilities here.
- `SimClock` (`include/core/sim_clock.h`) — fixed-timestep at 20 Hz; rendering is decoupled from sim ticks.
- `SimWorldBindings` (`include/sim/sim_world_bindings.h`) — pass-through bundle of pointers given to `SystemRegistry::initialize()`; avoids passing 15 individual store pointers to every system.
- `SimEventQueue` — one-way async communication between systems within a tick. Systems post events; downstream systems consume them.

### Memory rules

- SoA over AoS. Entity/tile links use flat `uint32_t` IDs — never raw or smart pointers between runtime entities.
- **No heap allocation inside `runTick()`** — no `new`, `push_back`, string concat. Pre-allocate at init or chunk-create time.
- UI panels: use `std::string_view` or static char buffers, not per-frame `std::string`.

## Code Style

| Style | Use for |
|-------|---------|
| PascalCase | Classes, structs, namespaces, enums |
| camelCase | Functions, methods, variables |
| ALL_CAPS | Macros, `constexpr` constants |
| snake_case | Source files, directories |

- Functions start with a verb: `getTileAt`, `initializeMap`, `canExtort`.
- Booleans use state prefixes: `isLoading`, `hasError`, `isPaused`.
- No magic numbers — use `constexpr` or scoped enums.
- Allowed abbreviations: `i/j/k`, `id`, `ctx`, `dt`, `io`, `err`.
- `const` by default; `constexpr` where possible.
- Early returns / guard clauses over deep nesting.
- Single logical operation per function (~30–40 lines soft limit; ImGui layout blocks are exempt).
- Composition over inheritance. No deep virtual hierarchies.
- Exceptions only for unrecoverable startup failures. Operational failures use error codes or `std::expected`.
- Validate in constructors/factories; keep processing code free of redundant checks.
- Declarations in `include/`, logic in `src/`. Never mix.
- `Core` namespace for framework code; `Utils` for generic helpers.

## Testing

Catch2 v3.5.2. Each test is its own CMake executable — add sources explicitly in `CMakeLists.txt`.

- Unit tests: Arrange–Act–Assert. Name inputs/outputs explicitly (`inputCoord`, `expectedRegion`).
- Integration tests: Given–When–Then structure.
- Every public utility in `Core::World` and `Core` sim types must have tests.

To add a new test: create `test/my_feature_test.cpp`, add a new `add_executable` block to `CMakeLists.txt` listing only the `.cpp` files that test needs, link `Catch2::Catch2WithMain`, call `catch_discover_tests`.

## Procgen

World generation is multi-pass and seed-driven (`uint64_t worldSeed`):
1. Land/water mask (coastline, Hudson, East River)
2. Borough + NJ region assignment
3. Terrain / elevation
4. Details (business nodes, vitality, landmarks)

Never write a monolithic generator function. Each pass is a separate function/module.
