#include <ctime>
#include <iostream>
#include <vector>

// Modular Headers
#include "../../include/AssetManager.hpp"
#include "../../include/BinaryExporter.hpp"
#include "../../include/Biology.hpp"
#include "../../include/Environment.hpp"
#include "../../include/Lore.hpp"
#include "../../include/Simulation.hpp"
#include "../../include/Terrain.hpp"
#include "../../include/WorldEngine.hpp"

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

  std::cout << "[LOG] Loading S.A.G.A. Map (data/world.map)...\n";
  if (!BinaryExporter::LoadWorld(buffers, "bin/data/world.map")) {
    if (!BinaryExporter::LoadWorld(buffers, "data/world.map")) {
      std::cout << "[ERROR] Could not find S.A.G.A. world data! Run Architect "
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

    if (year % 10 == 0) {
      std::cout << "[SIM] Decade " << year / 10 << " complete.\n";
    }
  }

  clock_t end = clock();
  double elapsed = (double)(end - start) / CLOCKS_PER_SEC;

  std::cout << "\n[SUCCESS] S.A.G.A. Simulation Finished in " << elapsed
            << "s.\n";
  std::cout << "[LOG] History compiled to logs/history.txt\n";

  // Cleanup
  finder.Cleanup(graph);
  buffers.Cleanup();

  return 0;
}
