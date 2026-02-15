#include "../../include/AssetManager.hpp"
#include "../../include/Lore.hpp"
#include "../../include/SagaConfig.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>


using json = nlohmann::json;

namespace LoreManager {

std::vector<WikiArticle> wikiDB;
std::vector<CategoryTemplate> templates;

void Load(const std::string &lorePath, const std::string &templPath) {
  std::cout << "[LORE] Loading database from: " << lorePath << "\n";
  std::ifstream f(lorePath);
  if (!f.is_open()) {
    std::cout << "[LORE] Failed to open " << lorePath << "\n";
    return;
  }

  try {
    json j;
    f >> j;
    wikiDB.clear();
    for (auto &item : j) {
      WikiArticle a;
      a.id = item.value("id", -1);
      a.title = item.value("title", "Untitled");
      a.isAgent = item.value("isAgent", false);
      a.isFaction = item.value("isFaction", false);
      a.hasLocation = item.value("hasLocation", false);
      a.mapX = item.value("x", 0);
      a.mapY = item.value("y", 0);
      if (item.contains("agentType"))
        a.agentType = (AgentType)item["agentType"];

      wikiDB.push_back(a);
    }
    std::cout << "[LORE] Successfully loaded " << wikiDB.size()
              << " articles.\n";
  } catch (const std::exception &e) {
    std::cout << "[LORE] JSON Error: " << e.what() << "\n";
  }
}

void Save(const std::string &lorePath, const std::string &templPath) {
  json j = json::array();
  for (const auto &a : wikiDB) {
    json item;
    item["id"] = a.id;
    item["title"] = a.title;
    item["isAgent"] = a.isAgent;
    item["isFaction"] = a.isFaction;
    item["hasLocation"] = a.hasLocation;
    item["x"] = a.mapX;
    item["y"] = a.mapY;
    if (a.isAgent)
      item["agentType"] = (int)a.agentType;
    j.push_back(item);
  }
  std::ofstream f(lorePath);
  f << j.dump(4);
}

void ExportGlobalState(const std::string &path, const WorldBuffers &buffers) {
  json root;

  // 1. Analyze World State (Macro Layer Analysis)
  bool isWar = false;
  bool isFamine = false;
  float totalPop = 0;
  float totalWealth = 0.0f;
  std::map<int, int> factionPower;

  for (size_t i = 0; i < buffers.count; ++i) {
    float pop = (float)buffers.population[i];
    totalPop += pop;

    // Hardcoded check for Food (0), Wood (1), Iron (2)
    float cellW = 0.0f;
    if (buffers.resourceInventory) {
      cellW += buffers.resourceInventory[i * 16 + 0] * 1.0f; // Food
      cellW += buffers.resourceInventory[i * 16 + 1] * 2.0f; // Wood
      cellW += buffers.resourceInventory[i * 16 + 2] * 5.0f; // Iron
    }
    totalWealth += cellW;

    if (i < 1000 && cellW > 0.1f) {
      static int logCount = 0;
      if (logCount < 10) {
        std::cout << "[DEBUG] Economic Activity detected at cell " << i
                  << ": Wealth=" << cellW << ", Pop=" << pop << "\n";
        logCount++;
      }
    }

    if (buffers.chaos[i] > 0.8f && buffers.agentStrength[i] > 200.0f)
      isWar = true;
    if (buffers.chaos[i] > 0.8f &&
        (buffers.resourceInventory ? buffers.resourceInventory[i * 16 + 0]
                                   : 0) < 10.0f) // Low Food
      isFamine = true;

    int fid = buffers.factionID[i];
    if (fid > 0)
      factionPower[fid] += (int)pop;
  }

  std::cout << "[DEBUG] Global State Scan Summary:\n";
  std::cout << "  - Total Population: " << totalPop << "\n";
  std::cout << "  - Total Wealth:     " << totalWealth << "\n";

  // 2. Build Meta Section
  root["meta"] = {
      {"epoch", LoreScribeNS::currentYear},
      {"global_pop", totalPop},
      {"global_wealth", totalWealth},
      {"flags", {{"IS_WAR_ACTIVE", isWar}, {"IS_FAMINE_ACTIVE", isFamine}}}};

  // 3. Serialize Factions
  root["factions"] = json::array();
  for (const auto &a : wikiDB) {
    if (a.isFaction) {
      json fObj;
      fObj["id"] = a.id;
      fObj["name"] = a.title;
      fObj["power"] = factionPower[a.id];
      fObj["year"] = a.formationYear;
      root["factions"].push_back(fObj);
    }
  }

  // 4. Serialize Important Nodes (Cities/Landmarks)
  root["nodes"] = json::array();
  for (const auto &a : wikiDB) {
    if (a.hasLocation) {
      json nObj;
      nObj["id"] = a.id;
      nObj["name"] = a.title;
      nObj["x"] = a.mapX;
      nObj["y"] = a.mapY;
      nObj["type"] = a.isFaction ? "CITY" : "LANDMARK";
      nObj["is_lore_site"] = true;
      root["nodes"].push_back(nObj);
    }
  }

  // 4a. Detect Emerged Settlements
  if (buffers.structureType) {
    for (uint32_t i = 0; i < buffers.count; ++i) {
      if (buffers.structureType[i] >= 2) { // Village or City
        json nObj;
        nObj["id"] = 20000 + i;
        nObj["name"] = (buffers.structureType[i] == 3) ? "Emergent City"
                                                       : "Emergent Village";
        nObj["x"] = (int)(i % 1000);
        nObj["y"] = (int)(i / 1000);
        nObj["type"] = "CITY";
        nObj["is_lore_site"] = false;
        root["nodes"].push_back(nObj);
      }
    }
  }

  std::ofstream out(path);
  out << root.dump(4);
}

WikiArticle *GetArticle(int id) {
  for (auto &a : wikiDB)
    if (a.id == id)
      return &a;
  return nullptr;
}

WikiArticle *GetArticleByTitle(const std::string &title) {
  for (auto &a : wikiDB)
    if (a.title == title)
      return &a;
  return nullptr;
}

std::vector<SyncError> ValidateSync(WorldBuffers &buffers) {
  return {}; // Stub for now
}

} // namespace LoreManager
