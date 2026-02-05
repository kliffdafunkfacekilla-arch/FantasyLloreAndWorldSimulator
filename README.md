# 🌍 Omnis World Engine

A high-performance procedural world simulator written in C++ with OpenGL visualization. Generates living, breathing fantasy worlds with terrain, climate, hydrology, and emergent civilizations on a **1,000,000 cell** grid.

![Status](https://img.shields.io/badge/status-active_development-brightgreen)
![Platform](https://img.shields.io/badge/platform-Windows-blue)
![License](https://img.shields.io/badge/license-MIT-blue)

---

## ✨ Current Features (v0.2)

### Terrain Generation
- **Fractal Brownian Motion (FBm)** for realistic jagged coastlines
- **Mountain Mask System** - independent mountain placement from continent shape
- **Domain Warping** - alien/organic world aesthetics
- **Terracing** - stepped canyon/mesa terrain effects
- **Island Mode** - forces ocean at map edges with smooth falloff
- **Heightmap Import** - load real-world or custom elevation data

### Climate & Weather
- **Latitudinal Temperature** - equator hot, poles cold
- **Altitude Cooling** - mountains are cold
- **Wind Advection** - heat/moisture blown across map by wind angle
- **Global Climate Sliders** - shift entire world warmer/colder/wetter/drier

### Hydrology
- **Double-Buffered Flow Simulation** - realistic water movement
- **Rainfall System** - moisture accumulates and flows downhill
- **River Formation** - emergent river networks from terrain

### Civilization
- **Faction Spawning** - seed civilizations on the map
- **Population Growth** - agents spread to habitable neighbors
- **Territory Expansion** - factions grow based on resources

### Simulation Controls
- **Time Scale** - 1x to 10x speed
- **Realtime Erosion** - watch mountains erode live
- **Play/Pause** - freeze or resume world simulation

---

## 🖥️ UI Overview

The engine uses a **tabbed ImGui interface**:

| Tab | Controls |
|-----|----------|
| **Architect** | Seed, continent size, sea level, mountains, warping, terracing, island mode |
| **Simulation** | Play/Pause, time scale, wind, temperature, moisture, erosion, factions |
| **Visuals** | Zoom level, point size, camera offset |

A single **"GENERATE NEW WORLD"** button regenerates terrain, builds the connectivity graph, applies erosion, and warms up hydrology.

---

## 🔧 Building (Windows)

### Prerequisites
- [MSYS2 MinGW64](https://www.msys2.org/) with `clang++` and OpenGL support
- Dependencies bundled in `/deps` (GLFW, GLEW, ImGui, FastNoiseLite)

### Build & Run
```batch
./build.bat
```

This will:
1. Clean old binaries
2. Copy shaders to `/bin`
3. Compile with Clang++
4. Launch the engine

---

## 📁 Project Structure

```
FantasyLloreAndWorldSimulator/
├── bin/                    # Output (exe, DLLs, shaders)
├── data/                   # Game data files
├── deps/                   # External libraries
│   ├── glfw/
│   ├── glew/
│   └── imgui/
├── include/                # Header files
│   ├── WorldEngine.hpp     # Master data structures
│   ├── SimulationModules.hpp
│   └── FastNoiseLite.h
├── shaders/                # GLSL shaders
│   ├── world.vert
│   └── world.frag
├── src/
│   ├── main.cpp            # Entry point & UI
│   ├── core/               # Core systems
│   │   ├── TerrainController.cpp
│   │   ├── NeighborFinder.cpp
│   │   └── SimulationLoop.cpp
│   ├── modules/            # Simulation modules
│   │   ├── HydrologySim.cpp
│   │   ├── ClimateSim.cpp
│   │   └── AgentSystem.cpp
│   └── io/                 # File I/O
│       ├── LoreScribe.cpp
│       └── PlatformUtils.cpp
└── saves/                  # World snapshots
```

---

## 🗺️ Roadmap

### ✅ Completed

| Phase | Feature | Status |
|-------|---------|--------|
| **1** | Core Memory (SoA Layout, 1M cells) | ✅ Done |
| **2** | OpenGL Visualization Pipeline | ✅ Done |
| **3** | Terrain & Geology (Noise, Heightmaps) | ✅ Done |
| **4** | Connectivity & Hydrology (Rivers) | ✅ Done |
| **5** | Civilization & Agents (Factions) | ✅ Done |

### 🔄 In Progress

| Phase | Feature | Status |
|-------|---------|--------|
| **6** | Economy & Resources | 🔄 Partial |
| **7** | Lore Export (CSV History) | 🔄 Basic |

### 📋 Planned

| Phase | Feature | Description |
|-------|---------|-------------|
| **8** | Conflict System | Wars, border pushing, battle resolution |
| **9** | Magic/Chaos Field | Fluid dynamics diffusion for fantasy element |
| **10** | Timelapse Replay | Buffer snapshots for history visualization |
| **11** | Wiki/Lore Export | Binary export for external lore tools |

---

## 🎮 Controls

| Action | How |
|--------|-----|
| Generate World | Click **GENERATE NEW WORLD** |
| Randomize | Click **Randomize Seed** then regenerate |
| Simulate | Go to **Simulation** tab, click **RESUME TIME** |
| Speed Up | Drag **Time Scale** to 10x |
| Island Map | Enable **Island Mode** in Architect tab |

---

## 📜 License

MIT License - See [LICENSE](LICENSE) for details.

---

## 🤝 Contributing

This is an active development project. Issues and PRs welcome!

---

*Built with C++, OpenGL, GLFW, GLEW, Dear ImGui, and FastNoiseLite*
