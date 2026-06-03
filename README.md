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

## Windows installer (easy install)

1. Install [Inno Setup 6](https://jrsoftware.org/isinfo.php) (optional; needed only to build the setup `.exe`).
2. From the repo root:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\build_installer.ps1
```

Outputs in `dist/`:

| File | Description |
|------|-------------|
| `CapitalVice-Setup-0.1.0.exe` | Full installer (Start menu + uninstaller) |
| `CapitalVice-Portable-0.1.0.zip` | Unzip and run `capital_vice.exe` (no Inno required) |

The game installs to `Program Files\Capital Vice`. **Saves and settings** go to `%LOCALAPPDATA%\CapitalVice\` so they stay writable without admin rights.

CMake package (no Inno):

```powershell
cmake --build build --config Release --target capital_vice
cpack -G ZIP
```

## UI docking

Panels are dockable (drag to edges to snap). Layout persists to `capitalvice_layout.ini`. Use **View → Reset Panel Layout** to restore defaults.

ImGui is fetched from the official `docking` branch via CMake FetchContent (first configure may take longer).
