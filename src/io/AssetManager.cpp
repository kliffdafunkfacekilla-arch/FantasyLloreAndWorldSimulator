#include "../../include/AssetManager.hpp"
#include "../../include/Lore.hpp"
#include "../../include/SagaConfig.hpp"
#include "../../include/SimulationModules.hpp"
#include "../../include/nlohmann/json.hpp"
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>

using json = nlohmann::json;

namespace AssetManager {
std::vector<PointOfInterest> poiList;
std::vector<AgentTemplate> speciesRegistry;
std::vector<ResourceDef> resourceRegistry;
std::vector<ChaosRule> chaosRules;
std::vector<City> cityRegistry;
std::vector<FactionData> factionRegistry;
std::vector<AgentDefinition> agentRegistry;
std::vector<BiomeDef> biomeRegistry;

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

  // Default Biomes
  if (biomeRegistry.empty()) {
    biomeRegistry.push_back({0,
                             "Deep Ocean",
                             {0.0f, 0.0f, 0.4f},
                             -1.0f,
                             2.0f,
                             -1.0f,
                             2.0f,
                             -1.0f,
                             0.3f,
                             0.0f});
    biomeRegistry.push_back({1,
                             "Ocean",
                             {0.0f, 0.2f, 0.6f},
                             -1.0f,
                             2.0f,
                             -1.0f,
                             2.0f,
                             0.3f,
                             0.5f,
                             0.0f});
    biomeRegistry.push_back({2,
                             "Beach",
                             {0.8f, 0.7f, 0.5f},
                             -1.0f,
                             2.0f,
                             -1.0f,
                             2.0f,
                             0.0f,
                             0.05f,
                             0.0f});
    biomeRegistry.push_back({3,
                             "Scorched",
                             {0.3f, 0.1f, 0.1f},
                             0.8f,
                             1.0f,
                             0.0f,
                             0.2f,
                             0.05f,
                             1.0f,
                             0.0f});
    biomeRegistry.push_back({4,
                             "Desert",
                             {0.9f, 0.8f, 0.5f},
                             0.6f,
                             1.0f,
                             0.0f,
                             0.3f,
                             0.05f,
                             1.0f,
                             0.0f});
    biomeRegistry.push_back({5,
                             "Savanna",
                             {0.7f, 0.8f, 0.3f},
                             0.6f,
                             1.0f,
                             0.3f,
                             0.5f,
                             0.05f,
                             1.0f,
                             0.0f});
    biomeRegistry.push_back({6,
                             "Tropical Rainforest",
                             {0.0f, 0.4f, 0.1f},
                             0.5f,
                             1.0f,
                             0.6f,
                             1.0f,
                             0.05f,
                             0.8f,
                             0.0f});
    biomeRegistry.push_back({7,
                             "Grassland",
                             {0.3f, 0.7f, 0.2f},
                             0.3f,
                             0.6f,
                             0.3f,
                             0.6f,
                             0.05f,
                             0.7f,
                             0.0f});
    biomeRegistry.push_back({8,
                             "Forest",
                             {0.1f, 0.6f, 0.1f},
                             0.2f,
                             0.6f,
                             0.5f,
                             0.9f,
                             0.05f,
                             0.7f,
                             0.0f});
    biomeRegistry.push_back({9,
                             "Temperate Rainforest",
                             {0.1f, 0.5f, 0.3f},
                             0.2f,
                             0.5f,
                             0.8f,
                             1.0f,
                             0.05f,
                             0.8f,
                             0.0f});
    biomeRegistry.push_back({10,
                             "Taiga",
                             {0.2f, 0.4f, 0.4f},
                             0.05f,
                             0.3f,
                             0.4f,
                             0.9f,
                             0.05f,
                             0.8f,
                             0.0f});
    biomeRegistry.push_back({11,
                             "Tundra",
                             {0.6f, 0.6f, 0.6f},
                             -0.2f,
                             0.05f,
                             0.0f,
                             1.0f,
                             0.05f,
                             0.8f,
                             0.0f});
    biomeRegistry.push_back({12,
                             "Snow",
                             {0.9f, 0.9f, 1.0f},
                             -1.0f,
                             -0.2f,
                             0.0f,
                             1.0f,
                             0.05f,
                             1.0f,
                             0.0f});
    biomeRegistry.push_back({13,
                             "Mountain",
                             {0.4f, 0.4f, 0.4f},
                             -1.0f,
                             1.0f,
                             -1.0f,
                             1.0f,
                             0.7f,
                             1.0f,
                             0.2f});
  }

  // Default Agents
  if (agentRegistry.empty()) {
    AgentDefinition human;
    human.id = 0;
    human.name = "Human";
    human.type = AgentType::CIVILIZED;
    human.color[0] = 0.8f;
    human.color[1] = 0.6f;
    human.color[2] = 0.6f;
    human.idealTemp = 0.5f;
    human.resilience = 0.3f;
    human.expansionRate = 0.05f;
    human.aggression = 0.5f;
    human.diet[0] = 1.0f;
    human.output[1] = 0.1f;
    agentRegistry.push_back(human);
  }

  SyncWithLore();

  std::cout << "[ASSETS] Initialized with " << resourceRegistry.size()
            << " resources, " << chaosRules.size() << " chaos rules, "
            << agentRegistry.size() << " agents.\n";
}

void SyncWithLore() {
  std::cout << "[SYNC] Merging Lore DB with Rules Asset Registry...\n";

  // 1. Sync Resources (Promotion)
  for (auto &article : LoreManager::wikiDB) {
    if (article.isResource) {
      // Check if already in registry by name
      bool exists = false;
      for (auto &res : resourceRegistry) {
        if (res.name == article.title) {
          exists = true;
          break;
        }
      }

      if (!exists && resourceRegistry.size() < WorldBuffers::MAX_RESOURCES) {
        ResourceDef nr;
        nr.id = (int)resourceRegistry.size();
        nr.name = article.title;
        nr.value = 1.0f;
        nr.isRenewable = true;
        nr.scarcity = 0.5f;
        nr.spawnsInForest = true; // Defaults
        nr.spawnsInMountain = false;
        nr.spawnsInDesert = false;
        nr.spawnsInOcean = false;
        resourceRegistry.push_back(nr);
        std::cout << "[SYNC] Promoted Lore Resource: " << nr.name
                  << " (SimID: " << nr.id << ")\n";
      }
    }
  }

  // 2. Sync Agents
  for (auto &article : LoreManager::wikiDB) {
    if (article.isAgent) {
      bool exists = false;
      for (auto &ag : agentRegistry) {
        if (ag.name == article.title) {
          exists = true;
          break;
        }
      }

      if (!exists) {
        AgentDefinition nag;
        nag.id = (int)agentRegistry.size();
        nag.name = article.title;
        nag.type = article.agentType;
        nag.color[0] = 0.5f;
        nag.color[1] = 0.5f;
        nag.color[2] = 0.5f;
        nag.idealTemp =
            article.minTemp + (article.maxTemp - article.minTemp) * 0.5f;
        nag.idealMoisture = article.minMoisture +
                            (article.maxMoisture - article.minMoisture) * 0.5f;
        nag.deadlyTempLow = article.minTemp - 0.2f;
        nag.deadlyTempHigh = article.maxTemp + 0.2f;
        nag.deadlyMoistureLow = article.minMoisture - 0.2f;
        nag.deadlyMoistureHigh = article.maxMoisture + 0.2f;
        nag.resilience = 0.5f;
        nag.expansionRate = article.expansion;
        nag.aggression = article.aggression;
        nag.foodRequirement = 1.0f;
        agentRegistry.push_back(nag);
        std::cout << "[SYNC] Mirrored Lore Agent: " << nag.name
                  << " (ID: " << nag.id << ")\n";
      }
    }
  }
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
  if (!f.is_open())
    return;

  try {
    json j;
    f >> j;

    if (j.contains("resources")) {
      resourceRegistry.clear();
      for (auto &item : j["resources"]) {
        ResourceDef r;
        r.id = item.value("id", (int)resourceRegistry.size());
        r.name = item.value("name", "Unknown");
        r.value = item.value("value", 1.0f);
        r.isRenewable = item.value("renewable", true);
        r.scarcity = item.value("scarcity", 0.5f);
        if (item.contains("biomes")) {
          r.spawnsInForest = item["biomes"].value("forest", false);
          r.spawnsInMountain = item["biomes"].value("mountain", false);
          r.spawnsInDesert = item["biomes"].value("desert", false);
          r.spawnsInOcean = item["biomes"].value("ocean", false);
        }
        resourceRegistry.push_back(r);
      }
    }

    if (j.contains("chaos")) {
      chaosRules.clear();
      for (auto &item : j["chaos"]) {
        ChaosRule c;
        c.name = item.value("name", "Unknown");
        c.effectType = (ChaosEffect)item.value("type", 0);
        c.probability = item.value("chance", 0.01f);
        c.severity = item.value("severity", 0.1f);
        c.minChaosLevel = item.value("threshold", 0.5f);
        chaosRules.push_back(c);
      }
    }

    if (j.contains("agents")) {
      agentRegistry.clear();
      for (auto &item : j["agents"]) {
        AgentDefinition a;
        a.id = item.value("id", (int)agentRegistry.size());
        a.name = item.value("name", "Unknown");
        a.type = (AgentType)item.value("type", 0);
        a.idealTemp = item.value("idealTemp", 0.5f);
        a.idealMoisture = item.value("idealMoisture", 0.5f);
        a.resilience = item.value("resilience", 0.2f);
        a.deadlyTempLow = item.value("minTemp", 0.0f);
        a.deadlyTempHigh = item.value("maxTemp", 1.0f);
        a.deadlyMoistureLow = item.value("minMoist", 0.0f);
        a.deadlyMoistureHigh = item.value("maxMoist", 1.0f);
        a.expansionRate = item.value("expansion", 0.1f);
        a.aggression = item.value("aggression", 0.1f);
        a.foodRequirement = item.value("food", 1.0f);

        if (item.contains("diet") && !item["diet"].is_null()) {
          for (auto &[key, val] : item["diet"].items())
            a.diet[std::stoi(key)] = val.get<float>();
        }
        if (item.contains("output") && !item["output"].is_null()) {
          for (auto &[key, val] : item["output"].items())
            a.output[std::stoi(key)] = val.get<float>();
        }

        if (item.contains("color") && item["color"].is_array()) {
          a.color[0] = item["color"][0];
          a.color[1] = item["color"][1];
          a.color[2] = item["color"][2];
        }
        agentRegistry.push_back(a);
      }
    }

    if (j.contains("biomes")) {
      biomeRegistry.clear();
      for (auto &item : j["biomes"]) {
        BiomeDef b;
        b.id = item.value("id", (int)biomeRegistry.size());
        b.name = item.value("name", "Unknown");
        if (item.contains("color") && item["color"].is_array()) {
          b.color[0] = item["color"][0];
          b.color[1] = item["color"][1];
          b.color[2] = item["color"][2];
        }
        b.minTemp = item.value("minTemp", -1.0f);
        b.maxTemp = item.value("maxTemp", 1.0f);
        b.minMoisture = item.value("minMoisture", -1.0f);
        b.maxMoisture = item.value("maxMoisture", 1.0f);
        b.minHeight = item.value("minHeight", -1.0f);
        b.maxHeight = item.value("maxHeight", 1.0f);
        b.scarcity = item.value("scarcity", 0.0f);
        biomeRegistry.push_back(b);
      }
    }

    std::cout << "[ASSETS] Successfully loaded rules from " << path << "\n";
  } catch (const std::exception &e) {
    std::cerr << "[ASSETS] JSON Error during LoadAll: " << e.what() << "\n";
  }
}

void SaveAll(const std::string &path) {
  json j;
  for (const auto &r : resourceRegistry) {
    json res;
    res["id"] = r.id;
    res["name"] = r.name;
    res["value"] = r.value;
    res["renewable"] = r.isRenewable;
    res["scarcity"] = r.scarcity;
    res["biomes"] = {{"forest", r.spawnsInForest},
                     {"mountain", r.spawnsInMountain},
                     {"desert", r.spawnsInDesert},
                     {"ocean", r.spawnsInOcean}};
    j["resources"].push_back(res);
  }
  for (const auto &c : chaosRules) {
    json ch;
    ch["name"] = c.name;
    ch["type"] = (int)c.effectType;
    ch["chance"] = c.probability;
    ch["severity"] = c.severity;
    ch["threshold"] = c.minChaosLevel;
    j["chaos"].push_back(ch);
  }
  for (const auto &a : agentRegistry) {
    json ag;
    ag["id"] = a.id;
    ag["name"] = a.name;
    ag["type"] = (int)a.type;
    ag["idealTemp"] = a.idealTemp;
    ag["idealMoist"] = a.idealMoisture;
    ag["resilience"] = a.resilience;
    ag["minTemp"] = a.deadlyTempLow;
    ag["maxTemp"] = a.deadlyTempHigh;
    ag["minMoist"] = a.deadlyMoistureLow;
    ag["maxMoist"] = a.deadlyMoistureHigh;
    ag["expansion"] = a.expansionRate;
    ag["aggression"] = a.aggression;
    ag["food"] = a.foodRequirement;
    ag["color"] = {a.color[0], a.color[1], a.color[2]};

    json diet;
    for (auto const &[rid, amt] : a.diet)
      diet[std::to_string(rid)] = amt;
    ag["diet"] = diet;
    json output;
    for (auto const &[rid, amt] : a.output)
      output[std::to_string(rid)] = amt;
    ag["output"] = output;

    j["agents"].push_back(ag);
  }
  for (const auto &b : biomeRegistry) {
    json bi;
    bi["id"] = b.id;
    bi["name"] = b.name;
    bi["color"] = {b.color[0], b.color[1], b.color[2]};
    bi["minTemp"] = b.minTemp;
    bi["maxTemp"] = b.maxTemp;
    bi["minMoisture"] = b.minMoisture;
    bi["maxMoisture"] = b.maxMoisture;
    bi["minHeight"] = b.minHeight;
    bi["maxHeight"] = b.maxHeight;
    bi["scarcity"] = b.scarcity;
    j["biomes"].push_back(bi);
  }

  std::ofstream o(path);
  if (o.is_open()) {
    o << j.dump(4);
    std::cout << "[ASSETS] Saved rules to " << path << "\n";
  } else {
    std::cerr << "[ASSETS] Failed to save to " << path << "\n";
  }
}

int GetResourceID(const std::string &name) {
  for (const auto &r : resourceRegistry)
    if (r.name == name)
      return r.id;
  return -1;
}

ResourceDef *GetResource(int id) {
  for (auto &r : resourceRegistry)
    if (r.id == id)
      return &r;
  return nullptr;
}

std::vector<Unit> activeUnits;
std::map<std::string, float> diplomacyMatrix;

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

void CreateNewResource() {
  if (resourceRegistry.size() >= WorldBuffers::MAX_RESOURCES) {
    std::cout << "[ASSETS] Max resources reached.\n";
    return;
  }
  ResourceDef r;
  r.id = (int)resourceRegistry.size();
  r.name = "New Resource";
  r.value = 1.0f;
  r.isRenewable = true;
  r.regenerationRate = 0.1f;
  r.scarcity = 0.5f;
  r.spawnsInForest = true;
  r.spawnsInMountain = false;
  r.spawnsInDesert = false;
  r.spawnsInOcean = false;
  resourceRegistry.push_back(r);
}

void CreateNewAgent() {
  AgentDefinition a;
  a.id = (int)agentRegistry.size();
  a.name = "New Agent";
  a.type = AgentType::FAUNA;
  a.color[0] = 0.5f;
  a.color[1] = 0.5f;
  a.color[2] = 0.5f;
  a.idealTemp = 0.5f;
  a.idealMoisture = 0.5f;
  a.deadlyTempLow = 0.0f;
  a.deadlyTempHigh = 1.0f;
  a.deadlyMoistureLow = 0.0f;
  a.deadlyMoistureHigh = 1.0f;
  a.resilience = 0.5f;
  a.expansionRate = 0.1f;
  a.aggression = 0.0f;
  a.foodRequirement = 1.0f;
  agentRegistry.push_back(a);
  std::cout << "[ASSETS] Created new agent: " << a.name << "\n";
}

void SaveSimulationState(const std::string &path, const WorldBuffers &buffers,
                         const WorldSettings &settings) {
  std::ofstream out(path, std::ios::binary);
  if (!out.is_open())
    return;
  uint32_t magic = 0x004d4e53;
  uint32_t version = 1;
  uint32_t count = buffers.count;
  out.write((char *)&magic, 4);
  out.write((char *)&version, 4);
  out.write((char *)&count, 4);
  out.write((char *)&settings, sizeof(WorldSettings));
  auto saveArr = [&](float *ptr, size_t size) {
    if (ptr)
      out.write((char *)ptr, size);
  };
  auto saveArrInt = [&](int *ptr, size_t size) {
    if (ptr)
      out.write((char *)ptr, size);
  };
  auto saveArrU = [&](uint32_t *ptr, size_t size) {
    if (ptr)
      out.write((char *)ptr, size);
  };
  saveArr(buffers.height, count * sizeof(float));
  saveArr(buffers.temperature, count * sizeof(float));
  saveArr(buffers.moisture, count * sizeof(float));
  saveArrU(buffers.population, count * sizeof(uint32_t));
  saveArrInt(buffers.factionID, count * sizeof(int));
  saveArrInt(buffers.cultureID, count * sizeof(int));
  saveArrInt(buffers.civTier, count * sizeof(int));
  saveArrInt(buffers.buildingID, count * sizeof(int));
  if (buffers.resourceInventory)
    out.write((char *)buffers.resourceInventory,
              count * WorldBuffers::MAX_RESOURCES * sizeof(float));
  std::cout << "[ASSETS] World State Saved: " << path << std::endl;
}

void LoadSimulationState(const std::string &path, WorldBuffers &buffers,
                         WorldSettings &settings) {
  std::ifstream in(path, std::ios::binary);
  if (!in.is_open())
    return;
  uint32_t magic, version, count;
  in.read((char *)&magic, 4);
  in.read((char *)&version, 4);
  in.read((char *)&count, 4);
  if (magic != 0x004d4e53)
    return;
  if (count != buffers.count)
    buffers.Initialize(count);
  in.read((char *)&settings, sizeof(WorldSettings));
  auto loadArr = [&](float *ptr, size_t size) {
    if (ptr)
      in.read((char *)ptr, size);
  };
  auto loadArrInt = [&](int *ptr, size_t size) {
    if (ptr)
      in.read((char *)ptr, size);
  };
  auto loadArrU = [&](uint32_t *ptr, size_t size) {
    if (ptr)
      in.read((char *)ptr, size);
  };
  loadArr(buffers.height, count * sizeof(float));
  loadArr(buffers.temperature, count * sizeof(float));
  loadArr(buffers.moisture, count * sizeof(float));
  loadArrU(buffers.population, count * sizeof(uint32_t));
  loadArrInt(buffers.factionID, count * sizeof(int));
  loadArrInt(buffers.cultureID, count * sizeof(int));
  loadArrInt(buffers.civTier, count * sizeof(int));
  loadArrInt(buffers.buildingID, count * sizeof(int));
  if (buffers.resourceInventory)
    in.read((char *)buffers.resourceInventory,
            count * WorldBuffers::MAX_RESOURCES * sizeof(float));
  std::cout << "[ASSETS] World State Loaded: " << path << std::endl;
}

float GetTotalCellWealth(uint32_t cellIdx, const WorldBuffers &b) {
  if (cellIdx >= b.count)
    return 0.0f;
  float total = 0.0f;
  // Calculate value based on inventory * resource registry values
  for (const auto &res : resourceRegistry) {
    float amt = b.GetResource(cellIdx, res.id);
    if (amt > 0.001f) {
      total += amt * res.value;
    }
  }
  return total;
}

} // namespace AssetManager
