#include "../../include/AssetManager.hpp"
#include "../../include/SimulationModules.hpp"
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>

namespace AssetManager {
std::vector<PointOfInterest> poiList;
std::vector<AgentTemplate> speciesRegistry;
std::vector<ResourceDef> resourceRegistry;
std::vector<ChaosRule> chaosRules;
std::vector<City> cityRegistry;
std::vector<FactionData> factionRegistry;
std::vector<AgentDefinition> agentRegistry;

void Initialize() {
  LoadAll();

  // DEFAULTS: If empty, create standard rules so the app works
  if (resourceRegistry.empty()) {
    resourceRegistry.push_back(
        {0, "Food", 1.0f, true, 0.1f, 0.0f, true, false, false, true});
    resourceRegistry.push_back(
        {1, "Wood", 2.0f, true, 0.05f, 0.2f, true, false, false, false});
    resourceRegistry.push_back(
        {2, "Iron", 5.0f, false, 0.0f, 0.8f, false, true, false, false});
    resourceRegistry.push_back(
        {3, "Gold", 10.0f, false, 0.0f, 0.95f, false, true, false, false});
  }

  if (chaosRules.empty()) {
    chaosRules.push_back(
        {"Mana Leak", ChaosEffect::MUTATE_LIFE, 0.01f, 0.5f, 0.2f});
    chaosRules.push_back(
        {"Reality Tear", ChaosEffect::SPAWN_RIFT, 0.001f, 1.0f, 0.8f});
    chaosRules.push_back(
        {"Resource Decay", ChaosEffect::DESTROY_RESOURCE, 0.05f, 0.3f, 0.5f});
  }

  // Default species
  if (speciesRegistry.empty()) {
    AgentTemplate human = {};
    human.id = 0;
    std::strncpy(human.name, "Human", 31);
    human.isStatic = false;
    human.canBuild = true;
    human.canWar = true;
    human.idealTemp = 0.5f;
    human.tempTolerance = 0.3f;
    human.reproductionRate = 0.05f;
    human.movementSpeed = 0.2f;
    human.aggression = 0.5f;
    human.strength = 1.0f;
    human.spawnsInForest = true;
    human.spawnsInMountain = false;
    speciesRegistry.push_back(human);

    AgentTemplate elf = {};
    elf.id = 1;
    std::strncpy(elf.name, "Elf", 31);
    elf.isStatic = false;
    elf.canBuild = true;
    elf.canWar = true;
    elf.idealTemp = 0.4f;
    elf.tempTolerance = 0.2f;
    elf.reproductionRate = 0.02f;
    elf.movementSpeed = 0.3f;
    elf.aggression = 0.2f;
    elf.strength = 0.8f;
    elf.spawnsInForest = true;
    speciesRegistry.push_back(elf);

    AgentTemplate tree = {};
    tree.id = 2;
    std::strncpy(tree.name, "Oak Tree", 31);
    tree.isStatic = true;
    tree.canBuild = false;
    tree.idealTemp = 0.5f;
    tree.tempTolerance = 0.3f;
    tree.reproductionRate = 0.1f;
    tree.resourceProduction[0] = 0.5f; // Food (fruits)
    tree.resourceProduction[1] = 2.0f; // Wood
    tree.spawnsInForest = true;
    speciesRegistry.push_back(tree);
  }

  std::cout << "[ASSETS] Initialized with " << resourceRegistry.size()
            << " resources, " << chaosRules.size() << " chaos rules, "
            << speciesRegistry.size() << " species.\n";
}

void RegisterCity(int location, int faction, int year) {
  City c;
  c.id = (int)cityRegistry.size();
  c.locationIndex = location;
  c.factionID = faction;
  c.population = 1000;
  c.yearFounded = year;
  c.name = NameGenerator::GenerateCityName();

  cityRegistry.push_back(c);

  std::string msg = "The city of " + c.name + " has been founded.";
  LoreScribeNS::LogEvent(year, "FOUNDING", location, msg);
  std::cout << "[CITY] " << msg << "\n";
}

void LoadAll(const std::string &path) {
  std::ifstream f(path);
  if (!f.is_open()) {
    std::cout << "[ASSETS] No rules.json found, using defaults.\n";
    return;
  }

  std::cout << "[ASSETS] Loading rules from " << path << "\n";

  // Simple line-by-line parsing (avoiding json.hpp dependency for now)
  // Full JSON parsing can be added later with nlohmann/json
  std::string line;
  while (std::getline(f, line)) {
    // Basic parsing can be implemented here
  }
  f.close();
}

void SaveAll(const std::string &path) {
  std::ofstream o(path);
  if (!o.is_open()) {
    std::cout << "[ASSETS] Failed to save to " << path << "\n";
    return;
  }

  // Write simple JSON format
  o << "{\n";

  // Resources
  o << "  \"resources\": [\n";
  for (size_t i = 0; i < resourceRegistry.size(); ++i) {
    const auto &r = resourceRegistry[i];
    o << "    {";
    o << "\"id\":" << r.id << ",";
    o << "\"name\":\"" << r.name << "\",";
    o << "\"value\":" << r.value << ",";
    o << "\"renewable\":" << (r.isRenewable ? "true" : "false") << ",";
    o << "\"scarcity\":" << r.scarcity << ",";
    o << "\"biomes\":{";
    o << "\"forest\":" << (r.spawnsInForest ? "true" : "false") << ",";
    o << "\"mountain\":" << (r.spawnsInMountain ? "true" : "false") << ",";
    o << "\"desert\":" << (r.spawnsInDesert ? "true" : "false") << ",";
    o << "\"ocean\":" << (r.spawnsInOcean ? "true" : "false");
    o << "}}";
    if (i < resourceRegistry.size() - 1)
      o << ",";
    o << "\n";
  }
  o << "  ],\n";

  // Chaos Rules
  o << "  \"chaos\": [\n";
  for (size_t i = 0; i < chaosRules.size(); ++i) {
    const auto &c = chaosRules[i];
    o << "    {";
    o << "\"name\":\"" << c.name << "\",";
    o << "\"type\":" << (int)c.effectType << ",";
    o << "\"chance\":" << c.probability << ",";
    o << "\"severity\":" << c.severity << ",";
    o << "\"threshold\":" << c.minChaosLevel;
    o << "}";
    if (i < chaosRules.size() - 1)
      o << ",";
    o << "\n";
  }
  o << "  ]\n";

  o << "}\n";
  o.close();

  std::cout << "[ASSETS] Saved rules to " << path << "\n";
}

int GetResourceID(const std::string &name) {
  for (const auto &r : resourceRegistry) {
    if (r.name == name)
      return r.id;
  }
  return -1;
}

ResourceDef *GetResource(int id) {
  for (auto &r : resourceRegistry) {
    if (r.id == id)
      return &r;
  }
  return nullptr;
}

// --- MOBILE UNITS ---
std::vector<Unit> activeUnits;
std::map<std::string, float> diplomacyMatrix;

// Diplomacy key helper
std::string GetDiplomacyKey(int a, int b) {
  if (a > b)
    std::swap(a, b);
  return std::to_string(a) + "_" + std::to_string(b);
}

float GetRelation(int factionA, int factionB) {
  if (factionA == factionB)
    return 100.0f;
  std::string key = GetDiplomacyKey(factionA, factionB);
  auto it = diplomacyMatrix.find(key);
  if (it == diplomacyMatrix.end())
    return 0.0f;
  return it->second;
}

void SetRelation(int factionA, int factionB, float value) {
  diplomacyMatrix[GetDiplomacyKey(factionA, factionB)] = value;
}

void SpawnUnit(UnitType type, int faction, int startIdx, int targetIdx,
               int mapWidth) {
  if (startIdx < 0 || targetIdx < 0)
    return;

  Unit u;
  u.id = rand();
  u.type = type;
  u.factionID = faction;
  u.isAlive = true;

  u.x = (float)(startIdx % mapWidth);
  u.y = (float)(startIdx / mapWidth);
  u.targetX = targetIdx % mapWidth;
  u.targetY = targetIdx / mapWidth;

  u.resourceID = 0;
  u.resourceAmount = 0.0f;
  u.combatStrength = 10.0f;

  switch (type) {
  case UnitType::TRADER:
    u.speed = 1.0f;
    break;
  case UnitType::ARMY:
    u.speed = 0.5f;
    u.combatStrength = 50.0f;
    break;
  case UnitType::AIRSHIP:
    u.speed = 2.0f;
    break;
  default:
    u.speed = 0.5f;
  }

  activeUnits.push_back(u);
}

void CreateNewAgent() {
  AgentDefinition a;
  a.id = (int)agentRegistry.size();
  a.name = "New Agent";
  a.type = AgentType::FAUNA;
  a.color[0] = 0.5f;
  a.color[1] = 0.5f;
  a.color[2] = 0.5f;

  // Default biology
  a.idealTemp = 0.5f;
  a.idealMoisture = 0.5f;
  a.deadlyTempLow = 0.0f;
  a.deadlyTempHigh = 1.0f;
  a.deadlyMoistureLow = 0.0f;
  a.deadlyMoistureHigh = 1.0f;

  // Default behavior
  a.resilience = 0.5f;
  a.expansionRate = 0.1f;
  a.aggression = 0.0f;
  a.foodRequirement = 1.0f;

  agentRegistry.push_back(a);
  std::cout << "[ASSETS] Created new agent: " << a.name << " (ID: " << a.id
            << ")" << std::endl;
}

} // namespace AssetManager
