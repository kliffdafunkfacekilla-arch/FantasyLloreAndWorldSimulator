#pragma once
#include "WorldEngine.hpp"
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

namespace AssetManager {
// Registries
extern std::vector<PointOfInterest> poiList;
extern std::vector<AgentTemplate> speciesRegistry;
extern std::vector<ResourceDef> resourceRegistry;
extern std::vector<ChaosRule> chaosRules;
extern std::vector<City> cityRegistry;

void Initialize();

// IO Functions
void LoadAll(const std::string &path = "data/rules.json");
void SaveAll(const std::string &path = "data/rules.json");

// Helpers
int GetResourceID(const std::string &name);
ResourceDef *GetResource(int id);

// City Management
void RegisterCity(int location, int faction, int year);
} // namespace AssetManager
