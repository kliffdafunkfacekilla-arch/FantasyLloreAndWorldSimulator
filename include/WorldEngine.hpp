#pragma once

#include <algorithm>
#include <cmath>   // Added for sqrt
#include <cstdint> // Added for uint32_t
#include <map>
#include <string>
#include <vector>


// --- BIOME ENUM (Whittaker-inspired) ---
enum BiomeType {
  OCEAN = 0,
  SCORCHED,
  BARE,
  TUNDRA,
  SNOW, // Extreme Heat/Cold
  TEMPERATE_DESERT,
  SHRUBLAND,
  GRASSLAND, // Dry
  TEMPERATE_DECIDUOUS_FOREST,
  TEMPERATE_RAIN_FOREST, // Temperate Wet
  SUBTROPICAL_DESERT,
  TROPICAL_SEASONAL_FOREST,
  TROPICAL_RAIN_FOREST // Hot Wet
};

// --- AGENT TYPE ENUM ---
enum class AgentType {
  FLORA,    // Plants: Spreads by seeds, terraforms climate
  FAUNA,    // Animals: Migrates, consumes flora/fauna
  CIVILIZED // Factions: Builds structures, trades, wars
};

// --- AGENT & FACTION TEMPLATES ---

struct AgentTemplate {
  int id;
  char name[32]; // Fixed size for easy binary IO

  // --- TYPE ---
  AgentType type;        // FLORA, FAUNA, or CIVILIZED
  bool isStatic;         // TRUE = Plant/Fungi, FALSE = Animal/Monster
  bool canBuild;         // Can build cities (Civ)
  bool canWar;           // Combat unit
  bool isDomesticatable; // Can be tamed by civilizations

  // --- APPEARANCE ---
  float color[3]; // RGB for map rendering

  // --- BIOLOGY ---
  float idealTemp;        // 0.0 (Polar) to 1.0 (Desert)
  float tempTolerance;    // How flexible (0.1=Picky, 0.5=Hardy)
  float reproductionRate; // Plants: Growth / Animals: Birth rate
  float lifespan;         // In ticks (approx)
  float adaptiveRate;     // How fast they change idealTemp
  float climateEffect;    // How much this modifies temp/moisture per tick

  // --- BEHAVIOR ---
  float movementSpeed; // 0.0 for plants, 1.0 = moves 1 tile/tick
  float aggression;    // 0.0 = Pacifist, 1.0 = Attacks anything
  float strength;      // Combat power / Grazing resistance
  float intelligence;  // Bonus to Tech/Defense
  float sociality;     // Bonus to Pop Growth/Trade

  // --- DIET / ECONOMY ---
  // 0:Food, 1:Wood, 2:Iron, 3:Magic
  float resourceNeeds[4];       // Legacy: What they need
  float resourceProduction[4];  // What they MAKE (Plants make Food/Wood)
  float resourceConsumption[4]; // What they EAT (Animals eat Food)

  // --- BIOME SPAWN ---
  bool spawnsInForest;
  bool spawnsInMountain;
  bool spawnsInDesert;
  bool spawnsInOcean;
};

struct FactionData {
  int id;
  std::string name;
  float color[3]; // RGB for map rendering

  // Cultural Identity
  int coreCultureID; // Founding species (links to AgentTemplate)

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

// --- NEW: AgentDefinition (for EditorUI) ---
struct AgentDefinition {
  int id;
  std::string name;

  // Type
  AgentType type;

  // Appearance
  float color[3];

  // Biology - Temperature
  float idealTemp;      // Perfect temperature (0-1)
  float idealMoisture;  // Perfect moisture (0-1)
  float deadlyTempLow;  // Die below this
  float deadlyTempHigh; // Die above this
  float deadlyMoistureLow;
  float deadlyMoistureHigh;

  // Behavior
  float resilience;      // Resistance to death
  float expansionRate;   // How fast they spread
  float aggression;      // Combat tendency
  float foodRequirement; // How much food per tick

  // Economy - Resource desires: -1=avoid, 0=neutral, +1=seek
  std::map<int, float> diet;   // What they consume
  std::map<int, float> output; // What they produce
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
  float seaLevel = 0.4f;         // Defined Sea Level (0.0-1.0)
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

  // --- CLIMATE CONTROLS ---
  // Temperature Zones: 0=Polar, 1=Temperate, 2=Tropical
  float tempZonePolar = 0.0f;     // Temp at poles
  float tempZoneTemperate = 0.5f; // Temp at mid-latitudes
  float tempZoneTropical = 1.0f;  // Temp at equator

  // Wind Zones (5 Directions: N, NE, E, SE, S)
  float windZonesDir[5] = {3.14f / 2.0f, 3.14f / 4.0f, 0.0f, -3.14f / 4.0f,
                           -3.14f / 2.0f}; // Rads
  float windZonesStr[5] = {0.5f, 0.5f, 1.0f, 0.5f, 0.5f};

  float globalRainfall = 1.0f;   // User requested "1 over all rainfall"
  float rainfallModifier = 1.0f; // Kept for compatibility/slider
  float globalTempModifier = 1.0f;
  float globalWindStrength = 1.0f;

  // --- NEW CLIMATE ENGINE SETTINGS ---
  float globalTemp = 0.0f;  // -1.0 (Ice Age) to +1.0 (Global Warming)
  float windAngle = 0.785f; // Radians (approx 45 degrees / NE)
  float raininess = 1.0f;   // Rainfall multiplier

  // River Controls
  int riverCount = 50;
  float riverMaxSize = 5.0f;    // Max width/volume
  bool riverFlowToOcean = true; // Force routing
  bool showIce = true;

  // Island Mode
  bool islandMode = false;
  float edgeFalloff = 0.15f;
  float islandMargin = 50.0f;

  // Erosion
  int erosionIterations = 10;

  // View & Scale
  int zoomLevel = 1;
  float pointSize = 1.0f;
  float viewOffset[2] = {0.0f, 0.0f};

  // --- DISASTER CONTROLS ---
  struct DisasterSetting {
    bool enabled = true;
    float frequency = 0.001f; // Probability per tick
    float strength = 0.5f;
  };

  DisasterSetting quakeSettings;
  DisasterSetting tsunamiSettings;
  DisasterSetting tornadoSettings;
  DisasterSetting hurricaneSettings;
  DisasterSetting wildfireSettings;
  DisasterSetting floodSettings;
  DisasterSetting droughtSettings;

  // Factions & Biology
  bool enableFactions = true;
  bool enableBiology = true;  // Simulate vegetation/wildlife
  bool enableConflict = true; // Enable war/border pushing

  // --- SIMULATION CONTROLS ---
  int timeScale = 1;                  // Ticks per frame (Speed)
  bool enableRealtimeErosion = false; // Does wind/rain erode land live?
  float erosionRate = 0.01f;          // How fast mountains melt
};

// 2. The Million-Cell Memory (SoA Layout)
struct WorldBuffers {
  // Core Geometry (Always Allocated)
  float *posX = nullptr;
  float *posY = nullptr;
  float *height = nullptr;

  // Simulation Layers (Allocated on Demand)
  float *temperature = nullptr;
  float *moisture = nullptr;
  int *biomeID = nullptr;    // NEW: Whittaker classification
  float *windDX = nullptr;   // Wind Vector X
  float *windDY = nullptr;   // Wind Vector Y
  float *flux = nullptr;     // River/Water accumulation volume (READ buffer)
  float *nextFlux = nullptr; // Double-buffer for water (WRITE buffer)

  // Civilization & Life
  int *factionID = nullptr;
  int *cultureID =
      nullptr; // Species/ethnicity (separate from political faction)
  uint32_t *population = nullptr;
  float *chaos = nullptr;
  float *infrastructure = nullptr; // Roads/Cities
  float *wealth = nullptr;         // Accumulated resources (Food/Iron/Gold)

  // --- CONSTRUCTION LAYER ---
  uint8_t *structureType = nullptr;
  float *defense = nullptr; // Calculated defense (Walls + Terrain)

  // --- ECONOMY LAYER ---
  int *civTier = nullptr;
  int *buildingID = nullptr;          // Which building type is here
  float *resourceInventory = nullptr; // Flattened [cellIdx * 8 + resID]
  static const int MAX_RESOURCES = 16;

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

    temperature = new float[count];
    moisture = new float[count];
    biomeID = new int[count];
    windDX = new float[count];
    windDY = new float[count];
    factionID = new int[count];
    cultureID = new int[count];
    population = new uint32_t[count];
    chaos = new float[count];
    infrastructure = new float[count];

    // Zero out memory
    std::fill_n(factionID, count, 0);
    std::fill_n(cultureID, count, -1);
    std::fill_n(population, count, 0);
    std::fill_n(chaos, count, 0.0f);
    std::fill_n(infrastructure, count, 0.0f);
    std::fill_n(temperature, count, 0.0f);
    std::fill_n(moisture, count, 0.0f);
    std::fill_n(biomeID, count, 0);
    std::fill_n(windDX, count, 0.0f);
    std::fill_n(windDY, count, 0.0f);

    wealth = new float[count];
    std::fill_n(wealth, count, 0.0f);

    structureType = new uint8_t[count];
    std::fill_n(structureType, count, (uint8_t)0);
    defense = new float[count];
    std::fill_n(defense, count, 0.0f);

    flux = new float[count];            // Allocate Flux
    std::fill_n(flux, count, 0.0f);     // Zero it out
    nextFlux = new float[count];        // Allocate NextFlux
    std::fill_n(nextFlux, count, 0.0f); // Zero it out

    // Economy layer
    civTier = new int[count];
    std::fill_n(civTier, count, 0);
    buildingID = new int[count];
    std::fill_n(buildingID, count, 0);
    resourceInventory = new float[count * MAX_RESOURCES];
    std::fill_n(resourceInventory, count * MAX_RESOURCES, 0.0f);

    // Grid Initialization
    int side = (int)std::sqrt(count);
    if (side > 0) {
      for (int i = 0; i < (int)count; ++i) {
        posX[i] = (float)(i % side) / (float)side;
        posY[i] = (float)(i / side) / (float)side;
      }
    }
  }

  void Cleanup() {
    delete[] posX;
    delete[] posY;
    delete[] height;
    delete[] temperature;
    delete[] moisture;
    delete[] biomeID;
    delete[] windDX;
    delete[] windDY;
    delete[] factionID;
    delete[] cultureID;
    delete[] population;
    delete[] chaos;
    delete[] infrastructure;
    delete[] wealth;
    delete[] structureType;
    delete[] defense;
    delete[] flux;
    delete[] nextFlux;
    delete[] civTier;
    delete[] buildingID;
    delete[] resourceInventory;

    posX = nullptr; // Safety flag
  }

  // --- RESOURCE HELPERS ---
  float GetResource(uint32_t cellIdx, int resID) const {
    if (!resourceInventory || resID < 0 || resID >= MAX_RESOURCES)
      return 0.0f;
    return resourceInventory[cellIdx * MAX_RESOURCES + resID];
  }

  void AddResource(uint32_t cellIdx, int resID, float amount) {
    if (!resourceInventory || resID < 0 || resID >= MAX_RESOURCES)
      return;
    float &val = resourceInventory[cellIdx * MAX_RESOURCES + resID];
    val += amount;
    if (val < 0.0f)
      val = 0.0f; // No negative resources
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
