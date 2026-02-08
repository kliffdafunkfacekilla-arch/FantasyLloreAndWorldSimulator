#pragma once
#include "WorldEngine.hpp"
#include <map>
#include <string>
#include <vector>

// --- 1. RESOURCE DEFINITION ---
struct ResourceDef {
  int id;
  std::string name;
  float value;            // Economic worth
  bool isRenewable;       // True = Grows back (Wood), False = Finite (Gold)
  float regenerationRate; // How fast it comes back
  float scarcity;         // 0.0 (Common) to 1.0 (Rare)

  // Biome Requirement
  bool spawnsInForest;
  bool spawnsInMountain;
  bool spawnsInDesert;
  bool spawnsInOcean;
};

// --- 2. CHAOS RULE ---
enum class ChaosEffect {
  NONE,
  SPAWN_RIFT,
  MUTATE_LIFE,
  DESTROY_RESOURCE,
  ALTER_CLIMATE
};

struct ChaosRule {
  std::string name;
  ChaosEffect effectType;
  float probability;   // 0.0 - 1.0 chance per tick
  float severity;      // How strong the effect is
  float minChaosLevel; // Only triggers if cell chaos > this
};

// --- 3. POINT OF INTEREST ---
struct PointOfInterest {
  std::string name;
  int id;
  int x, y;
  float radius;
  std::string description;
  bool trackEvents;
};

// --- 4. CITY ---
struct City {
  int id;
  std::string name;
  int locationIndex;
  int factionID;
  int population;
  int yearFounded;
};

// --- 5. MOBILE UNITS ---
enum class UnitType {
  SETTLER, // Creates new cities
  TRADER,  // Moves resources
  ARMY,    // Conquers tiles
  AIRSHIP  // Ignores terrain, fast
};

struct Unit {
  int id;
  UnitType type;
  int factionID;

  // Position (float for smooth movement)
  float x, y;
  int targetX, targetY;

  // Stats
  float speed;
  float combatStrength;

  // Cargo (for traders/settlers)
  int resourceID;
  float resourceAmount;

  bool isAlive;
};

// --- 6. BIOME DEFINITION ---
struct BiomeDef {
  int id;
  std::string name;
  float color[3]; // RGB 0.0-1.0

  // Environmental Conditions (Min/Max)
  float minTemp, maxTemp;
  float minMoisture, maxMoisture;
  float minHeight, maxHeight;

  float scarcity; // 0.0 = Common, 1.0 = Rare (Checked first)
};

namespace AssetManager {
// Registries
extern std::vector<PointOfInterest> poiList;
extern std::vector<AgentTemplate> speciesRegistry;
extern std::vector<ResourceDef> resourceRegistry;
extern std::vector<ChaosRule> chaosRules;
extern std::vector<City> cityRegistry;
extern std::vector<FactionData> factionRegistry;

// NEW: Biomes
extern std::vector<BiomeDef> biomeRegistry;

// NEW: Agent Definitions (for EditorUI)
extern std::vector<AgentDefinition> agentRegistry;

// Mobile Units
extern std::vector<Unit> activeUnits;

// Diplomacy Matrix: Key = "factionA_factionB", Value = -100 to +100
extern std::map<std::string, float> diplomacyMatrix;

void Initialize();

// IO Functions
void LoadAll(const std::string &path = "data/rules.json");
void SaveAll(const std::string &path = "data/rules.json");

// Simulation State Save/Load
void SaveSimulationState(const std::string &path, const WorldBuffers &buffers,
                         const WorldSettings &settings);
void LoadSimulationState(const std::string &path, WorldBuffers &buffers,
                         WorldSettings &settings);

// Helpers
int GetResourceID(const std::string &name);
ResourceDef *GetResource(int id);

// City Management
void RegisterCity(int location, int faction, int year);

// Diplomacy Helpers
float GetRelation(int factionA, int factionB);
void SetRelation(int factionA, int factionB, float value);

// Unit Spawning
void SpawnUnit(UnitType type, int faction, int startIdx, int targetIdx,
               int mapWidth = 1000);

// Resource Editor
void CreateNewResource();

// Agent Editor
void CreateNewAgent();
} // namespace AssetManager
