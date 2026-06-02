# CapitalVice

Dwarf Fortress-style systems simulation set in NYC (five boroughs) + parts of NJ.

## Clone locally

Clone into a folder named **CapitalVice** (not `imgui-master`):

```bash
git clone https://github.com/tylerdukedev/MAFIA_MUD.git CapitalVice
cd CapitalVice
git checkout cursor/phase-0-1-bootstrap-d615
```

If you already have an old `imgui-master` folder, rename it or replace it with a fresh clone as above.

## Project layout

```
CapitalVice/          ← game project root (this repo)
├── include/          ← game headers
├── src/              ← game source
├── lib/imgui/        ← vendored Dear ImGui library (dependency only)
├── test/
└── docs/
```

Dear ImGui lives under `lib/imgui/` — that is a third-party UI library, not the game project itself.

## Build

```bash
CC=gcc CXX=g++ cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
ctest --test-dir build --output-on-failure
./build/capital_vice
```

See `docs/DEVELOPMENT_PLAN.md` for the phased roadmap.

## UI docking

Panels are dockable (drag to edges to snap). Layout persists to `capitalvice_layout.ini`. Use **View → Reset Panel Layout** to restore defaults.

ImGui is fetched from the official `docking` branch via CMake FetchContent (first configure may take longer).
