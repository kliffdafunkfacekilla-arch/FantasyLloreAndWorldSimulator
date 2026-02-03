#pragma once
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>


// 1. User Settings (From Menu)
struct WorldSettings {
  uint32_t cellCount = 1000000;
  int seed = 1337;
  float seaLevel = 0.2f;

  bool enableClimate = true;
  bool enableFactions = true;
  bool enableChaos = true;
  bool enableHydrology = true;

  uint32_t resourceCount = 4;

  // Height Controls
  float heightMultiplier = 1.0f;
  float heightMin = 0.0f;
  float heightMax = 1.0f;
  float heightSeverity = 1.0f;
  float featureFrequency = 0.01f;
  float featureClustering = 2.0f;

  // Climate
  float globalTempModifier = 1.0f;
  float rainfallAmount = 1.0f;
  float globalWindStrength = 1.0f;

  int worldScaleMode = 3;

  std::string heightmapPath = "";
  float chaosInstabilityScale = 0.5f;
};

// Resource Definition
struct ResourceRegistry {
  std::string name;
  int id;
  float baseScarcity;
  int preferredBiome;
};

// Universal Agent Template (DNA)
struct AgentTemplate {
  std::string speciesName;
  int id;

  // Capability Flags
  bool isStatic;         // True for Plants
  bool canBuild;         // True for People
  bool canWar;           // True for People
  bool isDomesticatable; // True for certain Animals

  // DNA / Attributes (The "Taming" Matrix)
  float aggression;   // 0.0 (Passive) to 1.0 (Hostile)
  float intelligence; // Higher = can be trained
  float sociality;    // Higher = herd animals (Easier to tame)
  float size;         // 0.1 (Mouse) to 1.0 (Elephant)

  // Biological Needs (Evolution Pressures)
  float idealTemp;
  float idealMoisture;
  float adaptiveRate; // How fast they evolve/drift

  // Resource Weights (What do they want?)
  // 0: Food, 1: Wood, 2: Iron, 3: Mana, etc.
  float weights[10];
};

// Faction System
struct FactionAssets {
  float transportSpeed = 1.0f;
  float combatBonus = 0.0f;
  std::vector<int> tamedSpeciesIDs;
};

struct Faction {
  int id;
  std::string name;
  FactionAssets assets;
};

struct ChronosConfig {
  float currentDayProgress = 0.0f;
  float dayLengthInSeconds = 1.0f;
  int dayCount = 0;
  int daysPerMonth = 30;
  int monthCount = 0;
  int yearCount = 0;

  float timeScale = 1.0f;
  int globalTimeScale = 1;
};

// 2. The Memory Buffers (SoA Style)
struct WorldBuffers {
  // Stored count for renderers/loops to use without needing Settings
  uint32_t count = 0;

  // Core Geometry
  float *posX = nullptr;
  float *posY = nullptr;
  float *height = nullptr;

  // Climate
  float *temperature = nullptr;
  // ^ Note: temperature is the variable name. User logic might refer to 'temp'
  // - be careful in implementation.
  float *moisture = nullptr;
  float *flux = nullptr;
  float *windDX = nullptr;
  float *windDY = nullptr;
  float *windStrength = nullptr;

  // Life & Civilization
  uint32_t *population = nullptr;
  int *speciesID = nullptr;
  int *factionID = nullptr;
  float *infrastructure = nullptr;

  float *resourceLevels = nullptr;
  float *chaosEnergy = nullptr;

  // Destructor to prevent memory leaks
  void clear() {
    delete[] posX;
    delete[] posY;
    delete[] height;
    delete[] temperature;
    delete[] moisture;
    delete[] flux;
    delete[] windDX;
    delete[] windStrength;
    delete[] windDY;
    delete[] population;
    delete[] speciesID;
    delete[] factionID;
    delete[] infrastructure;
    delete[] resourceLevels;
    delete[] chaosEnergy;
    std::cout << "[CLEANUP] World buffers deallocated." << std::endl;
  }
};
