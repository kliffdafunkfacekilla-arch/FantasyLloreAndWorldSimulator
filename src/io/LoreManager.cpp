#include "../../include/AssetManager.hpp"
#include "../../include/Lore.hpp"
#include "../../include/SagaConfig.hpp"
#include "../../include/nlohmann/json.hpp"
#include <fstream>
#include <iomanip>
#include <iostream>

using json = nlohmann::json;

namespace LoreManager {
std::vector<WikiArticle> wikiDB;
std::vector<CategoryTemplate> templates;

void Load(const std::string &lorePath, const std::string &templPath) {
  // 1. Templates
  std::ifstream ft(templPath);
  if (ft.is_open()) {
    try {
      json tRoot;
      ft >> tRoot;
      templates.clear();
      for (const auto &tObj : tRoot) {
        CategoryTemplate t;
        t.name = tObj.value("name", "Unknown");
        if (tObj.contains("fields")) {
          for (const auto &fObj : tObj["fields"])
            t.fields.push_back(
                {fObj.value("l", ""), (FieldType)fObj.value("t", 0)});
        }
        templates.push_back(t);
      }
    } catch (...) {
    }
  }
  if (templates.empty()) {
    templates.push_back({"General", {{"Description", FieldType::TEXT}}});
    templates.push_back(
        {"Faction",
         {{"Population", FieldType::NUMBER}, {"Color", FieldType::COLOR}}});
  }

  // 2. Articles
  std::ifstream f(lorePath);
  if (!f.is_open())
    return;
  try {
    json root;
    f >> root;
    wikiDB.clear();
    for (auto &obj : root) {
      WikiArticle a;
      a.id = obj.value("id", 0);
      a.title = obj.value("title", "Untitled");
      a.categoryName = obj.value("cat", "General");
      a.content = obj.value("content", "");
      if (obj.contains("data"))
        a.data = obj["data"].get<std::map<std::string, std::string>>();
      if (obj.contains("tags"))
        a.tags = obj["tags"].get<std::vector<std::string>>();
      a.simID = obj.value("simID", -1);
      a.imagePath = obj.value("img", "");

      if (obj.contains("loc")) {
        a.hasLocation = true;
        a.mapX = obj["loc"][0];
        a.mapY = obj["loc"][1];
      }

      a.isAgent = obj.value("isAgent", false);
      a.isBiome = obj.value("isBiome", false);
      a.isFaction = obj.value("isFaction", false);
      a.isResource = obj.value("isResource", false);

      if (a.isAgent && obj.contains("agent")) {
        auto &ag = obj["agent"];
        a.agentType = (AgentType)ag.value("type", 1);
        a.socialType = (SocialType)ag.value("social", 0);
        a.minTemp = ag.value("tMin", 0.0f);
        a.maxTemp = ag.value("tMax", 1.0f);
        a.minMoisture = ag.value("mMin", 0.0f);
        a.maxMoisture = ag.value("mMax", 1.0f);
        a.expansion = ag.value("exp", 0.1f);
        a.aggression = ag.value("agg", 0.1f);
        a.isTameable = ag.value("tame", false);
        a.isFarmable = ag.value("farm", false);
        if (ag.contains("rel")) {
          auto rels = ag["rel"].get<std::map<int, int>>();
          for (auto const &[id, r] : rels)
            a.resourceRelationships[id] = (ResourceRel)r;
        }
        if (ag.contains("biomePref"))
          a.biomePreferences = ag["biomePref"].get<std::map<int, float>>();
        if (ag.contains("harvest"))
          a.harvestOutput = ag["harvest"].get<std::map<int, float>>();
        if (ag.contains("living"))
          a.livingOutput = ag["living"].get<std::map<int, float>>();
      }

      if (a.isBiome && obj.contains("biome")) {
        auto &b = obj["biome"];
        a.biomeTempMod = b.value("tMod", 0.0f);
        a.biomeMoistureMod = b.value("mMod", 0.0f);
        if (b.contains("col")) {
          std::vector<float> c = b["col"].get<std::vector<float>>();
          a.biomeColor = ImVec4(c[0], c[1], c[2], (c.size() > 3 ? c[3] : 1.0f));
        }
      }

      if (a.isFaction && obj.contains("faction")) {
        auto &f = obj["faction"];
        a.formationYear = f.value("year", 0);
        a.formationCell = f.value("cell", -1);
        if (f.contains("events"))
          a.linkedEventIDs = f["events"].get<std::vector<int>>();
      }

      if (a.isResource && obj.contains("resource")) {
        auto &res = obj["resource"];
        a.scarcity = res.value("scarcity", 0.5f);
        a.isRenewable = res.value("renewable", true);
        if (res.contains("biomes"))
          a.spawnBiomeIDs = res["biomes"].get<std::vector<int>>();
      }

      wikiDB.push_back(a);
    }
  } catch (...) {
  }
}

void Save(const std::string &lorePath, const std::string &templPath) {
  json root = json::array();
  for (const auto &a : wikiDB) {
    json obj;
    obj["id"] = a.id;
    obj["title"] = a.title;
    obj["cat"] = a.categoryName;
    obj["content"] = a.content;
    obj["data"] = a.data;
    obj["tags"] = a.tags;
    obj["simID"] = a.simID;
    obj["img"] = a.imagePath;
    if (a.hasLocation)
      obj["loc"] = {a.mapX, a.mapY};
    obj["isAgent"] = a.isAgent;
    obj["isBiome"] = a.isBiome;
    obj["isFaction"] = a.isFaction;
    obj["isResource"] = a.isResource;

    if (a.isAgent) {
      obj["agent"] = {{"type", (int)a.agentType},
                      {"social", (int)a.socialType},
                      {"tMin", a.minTemp},
                      {"tMax", a.maxTemp},
                      {"mMin", a.minMoisture},
                      {"mMax", a.maxMoisture},
                      {"exp", a.expansion},
                      {"agg", a.aggression},
                      {"tame", a.isTameable},
                      {"farm", a.isFarmable},
                      {"rel", a.resourceRelationships},
                      {"biomePref", a.biomePreferences},
                      {"harvest", a.harvestOutput},
                      {"living", a.livingOutput}};
    }
    if (a.isBiome) {
      obj["biome"] = {
          {"tMod", a.biomeTempMod},
          {"mMod", a.biomeMoistureMod},
          {"col",
           {a.biomeColor.x, a.biomeColor.y, a.biomeColor.z, a.biomeColor.w}}};
    }
    if (a.isFaction) {
      obj["faction"] = {{"year", a.formationYear},
                        {"cell", a.formationCell},
                        {"events", a.linkedEventIDs}};
    }
    if (a.isResource) {
      obj["resource"] = {{"scarcity", a.scarcity},
                         {"renewable", a.isRenewable},
                         {"biomes", a.spawnBiomeIDs}};
    }
    root.push_back(obj);
  }
  std::ofstream o(lorePath);
  o << std::setw(4) << root;

  json tRoot = json::array();
  for (const auto &t : templates) {
    json tObj;
    tObj["name"] = t.name;
    json fields = json::array();
    for (const auto &f : t.fields)
      fields.push_back({{"l", f.label}, {"t", (int)f.type}});
    tObj["fields"] = fields;
    tRoot.push_back(tObj);
  }
  std::ofstream ot(templPath);
  ot << std::setw(4) << tRoot;
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
  std::vector<SyncError> errors;
  for (const auto &a : wikiDB) {
    if (!a.hasLocation)
      continue;
    int idx = a.mapY * 1000 + a.mapX;
    if (idx < 0 || idx >= (int)buffers.count) {
      errors.push_back({a.id, a.title, "Location out of bounds."});
      continue;
    }

    if (a.isFaction) {
      if (buffers.population[idx] == 0) {
        errors.push_back({a.id, a.title, "Lore site contains no population."});
      }
    }

    if (a.isAgent) {
      // Validation could check if the environment at idx is suitable for DNA
      float t = buffers.temperature[idx];
      float m = buffers.moisture[idx];
      if (t < a.minTemp || t > a.maxTemp || m < a.minMoisture ||
          m > a.maxMoisture) {
        errors.push_back(
            {a.id, a.title, "Environment is hostile to this agent's DNA."});
      }
    }
  }
  return errors;
  return errors;
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
    totalPop += buffers.population[i];
    totalWealth += buffers.wealth ? buffers.wealth[i] : 0.0f;
    if (buffers.chaos[i] > 0.8f && buffers.agentStrength[i] > 200.0f)
      isWar = true;
    if (buffers.chaos[i] > 0.8f &&
        buffers.GetResource(i, 0) < 10.0f) // Low Food
      isFamine = true;

    int fid = buffers.factionID[i];
    if (fid > 0)
      factionPower[fid] += (int)buffers.population[i];
  }

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
      fObj["power"] = factionPower[a.id]; // Derived from population map
      fObj["year"] = a.formationYear;
      root["factions"].push_back(fObj);
    }
  }

  // 4. Serialize Important Nodes (Cities/Landmarks)
  root["nodes"] = json::array();
  // ... (Would need to identify cities from WorldBuffers, currently just using
  // Lore nodes)
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

  // 4a. Detect Emerged Settlements (Not in Lore)
  if (buffers.structureType) {
    int side = 1000;
    for (uint32_t i = 0; i < buffers.count; ++i) {
      if (buffers.structureType[i] >= 2) { // Village or City
        // Check if a lore node already exists here (simple proximity check)
        bool exists = false;
        int cx = i % side;
        int cy = i / side;
        for (const auto &node : root["nodes"]) {
          int nx = node["x"];
          int ny = node["y"];
          if (abs(nx - cx) < 5 && abs(ny - cy) < 5) {
            exists = true;
            break;
          }
        }

        if (!exists) {
          json nObj;
          nObj["id"] = 10000 + i; // Offset ID for dynamic nodes
          nObj["name"] = (buffers.structureType[i] == 3) ? "Emergent City"
                                                         : "Young Village";
          nObj["x"] = cx;
          nObj["y"] = cy;
          nObj["type"] = (buffers.structureType[i] == 3) ? "CITY" : "VILLAGE";
          nObj["is_lore_site"] = false;
          nObj["wealth"] = buffers.wealth ? buffers.wealth[i] : 0.0f;
          nObj["infra"] =
              buffers.infrastructure ? buffers.infrastructure[i] : 0.0f;
          root["nodes"].push_back(nObj);

          // Jump ahead to avoid spamming the same city area
          i += 20;
        }
      }
    }
  }

  // 5. Write to File
  std::ofstream o(path);
  if (o.is_open()) {
    o << std::setw(4) << root;
  }
}

} // namespace LoreManager
