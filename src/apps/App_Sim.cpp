#include <ctime>
#include <direct.h>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

// Modular Headers
#include "../../include/AssetManager.hpp"
#include "../../include/BinaryExporter.hpp"
#include "../../include/Biology.hpp"
#include "../../include/Environment.hpp"
#include "../../include/Lore.hpp"
#include "../../include/SagaConfig.hpp"
#include "../../include/Simulation.hpp"
#include "../../include/Terrain.hpp"
#include "../../include/WorldEngine.hpp"
#include "../../include/nlohmann/json.hpp"

using json = nlohmann::json;

void ExportStoryHooks(const WorldBuffers &buffers, const std::string &path) {
  std::cout << "[LOG] Exporting Story Hooks to " << path << "...\n";
  json hooks = json::array();
  int side = 1000;

  for (int i = 0; i < (int)buffers.count; ++i) {
    bool isWar = buffers.agentStrength[i] > 200.0f;
    bool isFamine = buffers.chaos[i] > 0.8f;

    if (isWar || isFamine) {
      json hook;
      hook["location"] = {(float)(i % side), (float)i / (float)side};
      hook["type"] = isWar ? "WAR_FRONT" : "FAMINE";
      hooks.push_back(hook);

      // Skip nearby cells to avoid spamming the same event
      if (isWar)
        i += 50;
    }
  }

  std::ofstream file(path);
  if (file.is_open()) {
    file << hooks.dump(4);
    std::cout << "[SUCCESS] Exported " << hooks.size() << " hooks.\n";
  } else {
    std::cout << "[ERROR] Could not write hooks.json!\n";
  }
}

void ApplyWorldEdits(WorldBuffers &b, const std::string &path) {
  std::ifstream f(path);
  if (!f.is_open())
    return;

  try {
    json edits;
    f >> edits;
    f.close();

    for (auto &edit : edits) {
      if (edit["type"] == "CASUALTY") {
        int x = edit["data"]["x"];
        int y = edit["data"]["y"];
        float amt = edit["data"]["amount"];
        int idx = y * 1000 + x;

        if (idx >= 0 && idx < (int)b.count) {
          b.agentStrength[idx] *= (1.0f - amt);
          b.population[idx] = (uint32_t)(b.population[idx] * (1.0f - amt));
          b.chaos[idx] = std::min(1.0f, b.chaos[idx] + amt);

          std::cout << "[ORACLE] Applied casualty at " << x << "," << y
                    << " (Chaos: " << b.chaos[idx] << ")\n";
        }
      } else if (edit["type"] == "LOOT_DROP") {
        int x = edit["data"]["x"];
        int y = edit["data"]["y"];
        std::string item = edit["data"]["item"];

        std::cout << "[ORACLE] Relic registered at " << x << "," << y << ": "
                  << item << "\n";

        int idx = y * 1000 + x;
        LoreScribeNS::LogEvent(0, "RELIC_DISCOVERY", idx,
                               "Heroic artifact recovered: " + item);

        json eventData;
        eventData["item"] = item;
        eventData["x"] = x;
        eventData["y"] = y;
        LoreScribeNS::LogJsonEvent("RELIC_DISCOVERY", eventData);
      }
    }

    std::ofstream clear(path);
    clear << "[]";
  } catch (...) {
  }
}

int main() {
  std::cout << "========================================\n";
  std::cout << "   S.A.G.A. SIMULATION ENGINE v1.0      \n";
  std::cout << "========================================\n\n";

  // 1. Initialize Memory
  WorldBuffers buffers;
  buffers.Initialize(1000000); // 1 Million Cells
  WorldSettings settings;
  NeighborGraph graph;
  NeighborFinder finder;

  // 2. Load Simulation State
  std::cout << "[LOG] Loading S.A.G.A. Rules (" << SagaConfig::RULES_JSON
            << ")...\n";
  LoreManager::Load();
  AssetManager::Initialize();

  // 2.5 Prepare History Folder
  std::string historyPath = SagaConfig::DATA_HUB + "history";
  _mkdir(historyPath.c_str());

  std::cout << "[LOG] Loading S.A.G.A. Map (" << SagaConfig::DATA_HUB
            << "world.map)...\n";
  if (!BinaryExporter::LoadWorld(buffers, "bin/data/world.map")) {
    if (!BinaryExporter::LoadWorld(buffers,
                                   SagaConfig::DATA_HUB + "world.map")) {
      std::cout << "[ERROR] Could find S.A.G.A. world data! Run Architect "
                   "first.\n";
      return -1;
    }
  }

  std::cout << "[LOG] Synchronizing World Graphs...\n";
  finder.BuildGraph(buffers, buffers.count, graph);

  LoreScribeNS::Initialize();
  std::cout << "[LOG] Engine Hot and Ready. Seeding world...\n";
  AgentSystem::SpawnLife(buffers, 2000);       // 2000 flora/fauna nodes
  AgentSystem::SpawnCivilization(buffers, 25); // 25 starting civilizations

  // Jumpstart Economy: Give every cell some starting food/wood/stone
  for (uint32_t i = 0; i < buffers.count; ++i) {
    if (buffers.population[i] > 100) {
      buffers.AddResource(i, 0, 500.0f); // Food
      buffers.AddResource(i, 1, 200.0f); // Wood
      buffers.AddResource(i, 2, 50.0f);  // Iron/Stone
      buffers.wealth[i] = 1000.0f;       // Starting liquidity
    }
  }
  std::cout << "[LOG] Seeding complete and economy jumpstarted. Entering "
               "simulation loop.\n\n";

  // 3. Simulation Loop
  int totalYears = 100; // Historical Simulation Run
  int ticksPerYear = 12;

  std::cout << "[SIM] Starting simulation run (" << totalYears
            << " years)...\n";

  clock_t start = clock();

  for (int year = 1; year <= totalYears; ++year) {
    LoreScribeNS::currentYear = year;
    ApplyWorldEdits(buffers, SagaConfig::DATA_HUB + "world_edits.json");

    for (int month = 1; month <= ticksPerYear; ++month) {
      ClimateSim::Update(buffers, settings);
      HydrologySim::Update(buffers, graph, settings);

      AgentSystem::UpdateBiology(buffers, graph, settings);
      LogisticsSystem::Update(buffers, graph);
      ConflictSystem::Update(buffers, graph, settings);

      if (settings.enableFactions)
        AgentSystem::UpdateCivilization(buffers, graph);
      if (settings.enableConflict)
        ConflictSystem::Update(buffers, graph, settings);

      UnitSystem::Update(buffers, 100);
      CivilizationSim::Update(buffers, graph, settings);
    }

    // Save annual snapshot
    std::string snapshotName =
        SagaConfig::DATA_HUB + "history/year_" + std::to_string(year) + ".map";
    BinaryExporter::SaveWorld(buffers, snapshotName);

    if (year % 10 == 0) {
      std::cout << "[SIM] Decade " << year / 10
                << " complete. (Snapshot Saved: Year " << year << ")\n";
    } else if (year % 2 == 0) {
      std::cout << "[SIM] Year " << year << " / " << totalYears << "\r"
                << std::flush;
    }
  }

  clock_t end = clock();
  double elapsed = (double)(end - start) / CLOCKS_PER_SEC;

  std::cout << "\n[SUCCESS] S.A.G.A. Simulation Finished in " << elapsed
            << "s.\n";
  std::cout << "[LOG] History compiled to SAGA_Global_Data/history/\n";

  // --- EXPORT STORY HOOKS ---
  ExportStoryHooks(buffers, SagaConfig::DATA_HUB + "hooks.json");

  // --- EXPORT GLOBAL STATE (FLAS) ---
  std::cout << "[FLAS] Exporting Global State Graph to gamestate.json...\n";
  LoreManager::ExportGlobalState(SagaConfig::DATA_HUB + "gamestate.json",
                                 buffers);

  // Cleanup
  finder.Cleanup(graph);
  buffers.Cleanup();

  return 0;
}
