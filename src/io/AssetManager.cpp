#include "../../include/AssetManager.hpp"
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>


namespace AssetManager {
std::vector<PointOfInterest> poiList;
std::vector<AgentTemplate> speciesRegistry;
std::vector<ResourceDef> resourceRegistry;
std::vector<ChaosRule> chaosRules;

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

  std::cout << "[ASSETS] Initialized with " << resourceRegistry.size()
            << " resources and " << chaosRules.size() << " chaos rules.\n";
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
} // namespace AssetManager
