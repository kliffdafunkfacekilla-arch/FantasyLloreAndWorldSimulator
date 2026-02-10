#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>


namespace fs = std::filesystem;

// Modular Headers
#include "../../include/AssetManager.hpp"
#include "../../include/BinaryExporter.hpp"
#include "../../include/Biology.hpp"
#include "../../include/Environment.hpp"
#include "../../include/Lore.hpp"
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
      hook["location"] = {(float)(i % side), (float)(i / side)};
      hook["type"] = isWar ? "WAR_FRONT" : "FAMINE";
      hooks.push_back(hook);

      // Skip nearby cells to avoid spamming the same event
      // Simple heuristic: skip next 20 cells in the loop (approximate)
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
  std::cout << "[LOG] Loading S.A.G.A. Rules (data/rules.json)...\n";
  AssetManager::Initialize();

  // 2.5 Prepare History Folder
  if (!fs::exists("data/history")) {
    fs::create_directories("data/history");
  }

  std::cout << "[LOG] Loading S.A.G.A. Map (data/world.map)...\n";
  if (!BinaryExporter::LoadWorld(buffers, "bin/data/world.map")) {
    if (!BinaryExporter::LoadWorld(buffers, "data/world.map")) {
      std::cout << "[ERROR] Could find S.A.G.A. world data! Run Architect "
                   "first.\n";
      return -1;
    }
  }

  std::cout << "[LOG] Synchronizing World Graphs...\n";
  finder.BuildGraph(buffers, buffers.count, graph);

  LoreScribeNS::Initialize();
  std::cout << "[LOG] Engine Hot and Ready.\n\n";

  // 3. Simulation Loop
  int totalYears = 100;
  int ticksPerYear = 12;

  std::cout << "[SIM] Starting simulation run (" << totalYears
            << " years)...\n";

  clock_t start = clock();

  for (int year = 1; year <= totalYears; ++year) {
    for (int month = 1; month <= ticksPerYear; ++month) {
      ClimateSim::Update(buffers, settings);
      HydrologySim::Update(buffers, graph, settings);

      AgentSystem::UpdateBiology(buffers, settings);
      if (settings.enableFactions)
        AgentSystem::UpdateCivilization(buffers, graph);
      if (settings.enableConflict)
        ConflictSystem::Update(buffers, graph, settings);

      UnitSystem::Update(buffers, 100);
      CivilizationSim::Update(buffers, graph, settings);
    }

    // --- FLIGHT RECORDER ---
    // Save the state of the world this year so the Projector can watch it later
    std::string snapshotName =
        "data/history/year_" + std::to_string(year) + ".map";
    BinaryExporter::SaveWorld(buffers, snapshotName);

    if (year % 10 == 0) {
      std::cout << "[SIM] Decade " << year / 10
                << " complete. (Snapshot Saved)\n";
    }
  }

  clock_t end = clock();
  double elapsed = (double)(end - start) / CLOCKS_PER_SEC;

  std::cout << "\n[SUCCESS] S.A.G.A. Simulation Finished in " << elapsed
            << "s.\n";
  std::cout << "[LOG] History compiled to logs/history.txt\n";

  // --- EXPORT STORY HOOKS ---
  ExportStoryHooks(buffers, "data/hooks.json");

  // Cleanup
  finder.Cleanup(graph);
  buffers.Cleanup();

  return 0;
}
