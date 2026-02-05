#pragma once

#include <algorithm>
#include <cstdint> // Added for uint32_t
#include <string>
#include <vector>

// --- AGENT & FACTION TEMPLATES ---

struct AgentTemplate {
  int id;
  char name[32]; // Fixed size for easy binary IO

  // Capabilities
  bool isStatic; // Plant vs Animal
  bool canBuild; // Civilization builder
  bool canWar;   // Combat unit

  // Behavior (0.0 - 1.0)
  float aggression;   // Likelihood to attack neighbors
  float intelligence; // Bonus to Tech/Defense
  float sociality;    // Bonus to Pop Growth/Trade

  // Biology
  float idealTemp;        // 0.0 (Polar) to 1.0 (Desert)
  float adaptiveRate;     // How fast they change idealTemp
  float resourceNeeds[4]; // 0:Food, 1:Wood, 2:Iron, 3:Magic
};

struct FactionData {
  int id;
  std::string name;
  float color[3]; // RGB for map rendering

  // Cultural Profile
  float expansionDrive; // Speed of border push
  float aggression;     // Threshold for declaring war
  float techLevel;      // Multiplier for Defense

  // Assets (Tamed Wildlife)
  std::vector<int> tamedSpeciesIDs;

  // Stats
  long totalPopulation;
  int totalCells;
};

// 1. World Configuration (Linked to God Mode UI)
struct WorldSettings {
  // Core Generation
  uint32_t cellCount = 1000000;
  int seed = 1337;

  // Terrain & Import Controls
  char heightmapPath[256] = ""; // Fixed size for ImGui InputText compatibility
  // --- Advanced Terrain Controls ---
  float heightMultiplier = 1.0f; // Overall Vertical Scale
  float seaLevel = 0.2f;         // Defined Sea Level (0.0-1.0)
  float heightMin = 0.0f;
  float heightMax = 1.0f;

  // 1. Continent Shape (Base Layer)
  float continentFreq = 0.005f; // Low freq = Large Continents

  // 2. Mountain Detail (Ridged Layer)
  float featureFrequency = 0.02f; // High freq = Detailed peaks
  float mountainInfluence = 1.0f; // How much mountains rely on landmass

  // 3. Domain Warping (Alien/Fluid Look)
  float warpStrength = 1.0f; // Distortion amount

  // --- Physics Controls ---
  float heightSeverity =
      1.0f; // Exponent for slope steepness (NOT used for warp)
  float featureClustering = 2.0f; // Lacunarity // Noise frequency

  // Climate & Wind (5-Zone System)
  bool manualWindOverride = false;
  float windZones[5] = {0.5f, -0.5f, 1.0f, -0.5f, 0.5f}; // N.Pole to S.Pole
  float globalTempModifier = 1.0f;
  float rainfallModifier = 1.0f;
  float globalWindStrength = 1.0f;

  // Erosion
  int erosionIterations = 10;

  // View & Scale (Zoom System)
  int zoomLevel = 0;                  // 0=Global, 3=Local
  float pointSize = 1.0f;             // GPU point size
  float viewOffset[2] = {0.0f, 0.0f}; // Camera X,Y (Start at origin)

  // Factions & Biology
  bool enableFactions = true;
  bool enableBiology = true;  // Simulate vegetation/wildlife
  bool enableConflict = true; // Enable war/border pushing

  // UI/UX
  bool autoRegenerate = false; // Live preview mode
  float terraceSteps = 0.0f;   // 0 = off, 1-2 = stepped terrain

  // --- CLIMATE & WEATHER ---
  float windAngle = 0.0f;         // 0.0 to 6.28 (Radians)
  float windStrengthSim = 0.1f;   // How fast heat/moisture moves
  float globalTemperature = 0.0f; // -0.5 to 0.5 (Colder / Warmer)
  float globalMoisture = 0.0f;    // -0.5 to 0.5 (Drier / Wetter)

  // --- SIMULATION CONTROLS ---
  int timeScale = 1;                  // Ticks per frame (Speed)
  bool enableRealtimeErosion = false; // Does wind/rain erode land live?
  float erosionRate = 0.01f;          // How fast mountains melt

  // --- ISLAND MODE ---
  bool islandMode = false;   // Force ocean at map edges
  float edgeFalloff = 0.15f; // How wide the ocean border is (0.0-0.5)
};

// 2. The Million-Cell Memory (SoA Layout)
// using raw pointers for maximum CPU cache efficiency
struct WorldBuffers {
  // Core Geometry (Always Allocated)
  float *posX = nullptr;
  float *posY = nullptr;
  float *height = nullptr;

  // Simulation Layers (Allocated on Demand)
  float *temperature = nullptr;
  float *moisture = nullptr;
  float *windDX = nullptr;   // Wind Vector X
  float *windDY = nullptr;   // Wind Vector Y
  float *flux = nullptr;     // River/Water accumulation volume (READ buffer)
  float *nextFlux = nullptr; // Double-buffer for water (WRITE buffer)

  // Civilization & Life
  int *factionID = nullptr;
  uint32_t *population = nullptr;
  float *chaos = nullptr;
  float *infrastructure = nullptr; // Roads/Cities
  float *wealth = nullptr;         // Accumulated resources (Food/Iron/Gold)

  // Metadata
  uint32_t count = 0;

  // Lifecycle Management
  void Initialize(uint32_t c) {
    count = c;
    if (posX)
      Cleanup(); // Prevent double allocation

    posX = new float[count];
    posY = new float[count];
    height = new float[count];

    // Optional buffers can be initialized lazily,
    // but for now we allocate all for stability
    temperature = new float[count];
    moisture = new float[count];
    windDX = new float[count];
    windDY = new float[count];
    factionID = new int[count];
    population = new uint32_t[count];
    chaos = new float[count];
    infrastructure = new float[count];

    // Zero out memory
    std::fill_n(factionID, count, 0);
    std::fill_n(population, count, 0);
    std::fill_n(chaos, count, 0.0f);
    std::fill_n(infrastructure, count, 0.0f);
    std::fill_n(temperature, count, 0.0f);
    std::fill_n(moisture, count, 0.0f);
    std::fill_n(windDX, count, 0.0f);
    std::fill_n(windDY, count, 0.0f);

    wealth = new float[count];
    std::fill_n(wealth, count, 0.0f);

    flux = new float[count];            // Allocate Flux
    std::fill_n(flux, count, 0.0f);     // Zero it out
    nextFlux = new float[count];        // Allocate NextFlux
    std::fill_n(nextFlux, count, 0.0f); // Zero it out
  }

  void Cleanup() {
    delete[] posX;
    delete[] posY;
    delete[] height;
    delete[] temperature;
    delete[] moisture;
    delete[] windDX;
    delete[] windDY;
    delete[] factionID;
    delete[] population;
    delete[] chaos;
    delete[] infrastructure;
    delete[] wealth;
    delete[] flux;
    delete[] nextFlux;

    posX = nullptr; // Safety flag
  }
};

// 3. Time System
struct ChronosConfig {
  int dayCount = 0;
  int monthCount = 0;
  int yearCount = 0;
  int daysPerMonth = 30;
  float currentDayProgress = 0.0f;
  int globalTimeScale = 1; // 0 = Pause
};

// 4. Neighbor Graph (Added for Modules)
struct NeighborGraph {
  int *neighborData = nullptr;
  int *offsetTable = nullptr;
  uint8_t *countTable = nullptr;
};
