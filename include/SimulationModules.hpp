#pragma once

#include "WorldEngine.hpp"
#include <fstream>
#include <string>
#include <vector>

// --- Forward Declarations for Logic Modules ---

// 1. Terrain & Structure (src/core/TerrainController.cpp)
class TerrainController {
public:
  void LoadFromImage(const char *path, WorldBuffers &buffers);
  void ApplyThermalErosion(WorldBuffers &buffers, int iterations);
  void InvertHeights(WorldBuffers &buffers);
  // Helper to generate base noise if no image is present
  void GenerateProceduralTerrain(WorldBuffers &buffers,
                                 const WorldSettings &settings);
};

// 2. Climate Engine (src/modules/ClimateSim.cpp)
// Using a namespace/static approach for pure logic modules
namespace ClimateSim {
void Update(WorldBuffers &b, const WorldSettings &s);
}

// 2b. Hydrology (src/modules/HydrologySim.cpp)
namespace HydrologySim {
void Update(WorldBuffers &b, const NeighborGraph &g);
}

// 3. Main Simulation Loop (src/core/SimulationLoop.cpp)
class SimulationLoop {
public:
  void Update(WorldBuffers &b, ChronosConfig &clock, const WorldSettings &s);
};

// Helper struct for LoreScribe
struct LogEntry {
  std::string timestamp;
  std::string category;
  std::string message;
};

// 4. Lore & History (src/io/LoreScribe.cpp)
class LoreScribe {
private:
  std::ofstream historyFile;
  std::vector<LogEntry> sessionLogs;

public:
  void Initialize(std::string filename);
  void RecordEvent(const ChronosConfig &time, std::string category, int cellID,
                   std::string log);
  const std::vector<LogEntry> &GetRecentLogs();
};

// --- Additional Modules ---

// NeighborFinder
class NeighborFinder {
public:
  void BuildGraph(WorldBuffers &buffers, uint32_t count, NeighborGraph &graph);
  void Cleanup(NeighborGraph &graph);
};

// AgentSystem
namespace AgentSystem {
extern std::vector<AgentTemplate> speciesRegistry;
void Initialize();
void UpdateBiology(WorldBuffers &b, const WorldSettings &s);
void SpawnCivilization(WorldBuffers &b, int factionId);
void UpdateCivilization(WorldBuffers &b, const NeighborGraph &g);
} // namespace AgentSystem

// ConflictSystem
namespace ConflictSystem {
void Update(WorldBuffers &b, const NeighborGraph &g, const WorldSettings &s);
} // namespace ConflictSystem

// LoreScribe namespace (simpler API)
namespace LoreScribeNS {
void Initialize();
void LogEvent(int tick, const std::string &type, int location,
              const std::string &desc);
void Close();
extern int currentYear;
} // namespace LoreScribeNS

// ConflictSystem (Free function - legacy)
void ResolveConflicts(WorldBuffers &b, const NeighborGraph &graph);

// ChaosField namespace
namespace ChaosField {
void SpawnRift(WorldBuffers &b, int index, float intensity);
void ClearRifts();
void Update(WorldBuffers &b, const NeighborGraph &g);
} // namespace ChaosField

// ChaosField (Free function - legacy)
void UpdateChaos(WorldBuffers &b, const NeighborGraph &graph,
                 float diffusionRate);

// LogisticsSystem namespace
namespace LogisticsSystem {
void Update(WorldBuffers &b, const NeighborGraph &g);
} // namespace LogisticsSystem

// LogisticsSystem (Free function - legacy)
void ProcessLogistics(WorldBuffers &b, uint32_t count);

// IO Helpers
void LoadHeightmapData(const char *path, WorldBuffers &buffers, uint32_t count);

// Platform Utilities
namespace PlatformUtils {
std::string OpenFileDialog();
}
