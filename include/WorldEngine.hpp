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

  // Controls (legacy & param sync)
  float heightSeverity = 1.0f;
  float featureFrequency = 0.01f;
  float featureClustering = 2.0f;

  float severity = 1.0f;   // Sync with heightSeverity
  float frequency = 0.01f; // Sync with featureFrequency

  // Climate
  float globalTempModifier = 1.0f;
  float rainfallAmount = 1.0f;
  float rainfallModifier = 1.0f;
  float globalWindStrength = 1.0f;

  int worldScaleMode = 3;

  // New Parameter-Driven Fields
  char heightmapPath[256] = "";
  bool manualWindOverride = false;
  float windZoneWeights[5] = {0.5f, 0.5f, 0.5f, 0.5f,
                              0.5f}; // Strength/Dir for 5 zones
  int erosionIterations = 10;

  float chaosInstabilityScale = 0.5f;

  // Civilization & Logic
  float aggressionMult = 1.0f;
  bool peaceMode = false;
  float logisticsSpeed = 0.5f;
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

  // DNA / Attributes
  float aggression;
  float intelligence;
  float sociality;
  float size;

  // Biological Needs
  float idealTemp;
  float idealMoisture;
  float adaptiveRate;

  // Resource Weights
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
  uint32_t count = 0;

  // Core Geometry
  float *posX = nullptr;
  float *posY = nullptr;
  float *height = nullptr;

  // Climate
  float *temperature = nullptr;
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
