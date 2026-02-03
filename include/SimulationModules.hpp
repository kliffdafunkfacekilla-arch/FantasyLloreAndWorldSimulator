#pragma once
#include "WorldEngine.hpp"
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

// Connectivity
struct NeighborGraph {
  int *neighborData;
  int *offsetTable;
  uint8_t *countTable;
};

class NeighborFinder {
public:
  void BuildGraph(WorldBuffers &buffers, uint32_t count, NeighborGraph &graph);
  void Cleanup(NeighborGraph &graph);
};

class TerrainController {
public:
  void GenerateTerrain(WorldBuffers &buffers, const WorldSettings &settings);
  void RunThermalErosion(WorldBuffers &buffers, const NeighborGraph &graph,
                         int iterations, uint32_t count);
  // User Helpers
  void LoadFromImage(const char *path, WorldBuffers &buffers);
  void ApplyThermalErosion(WorldBuffers &buffers, const NeighborGraph &graph);
};

class HydrologySim {
public:
  void CalculateFlow(WorldBuffers &buffers, const NeighborGraph &graph,
                     uint32_t count);
};

class ClimateSim {
public:
  void UpdateGlobalClimate(WorldBuffers &buffers,
                           const WorldSettings &settings);
  void GenerateBiomes(WorldBuffers &buffers, WorldSettings &settings,
                      uint32_t count);
};

class LifeSim {
public:
  void SeedSpecies(WorldBuffers &buffers, const AgentTemplate &dna,
                   uint32_t startCell);
  void Evolve(AgentTemplate &dna, float cellTemp, float cellMoisture);
};

struct AgentInstance {
  uint32_t cellID;
  int speciesID;
  float population;
  float fitness;
};

class AgentSystem {
public:
  void TickAgents(WorldBuffers &buffers, std::vector<AgentTemplate> &registry,
                  float deltaTime, uint32_t count);

private:
  void HandleConstruction(uint32_t cellID, WorldBuffers &buffers,
                          AgentTemplate &dna);
  void HandleBorderExpansion(uint32_t cellID, WorldBuffers &buffers,
                             AgentTemplate &dna);
};

namespace FactionLogic {
void ProcessDomestication(Faction &fact, AgentInstance &animal,
                          const std::vector<AgentTemplate> &registry);
}

// Lore Systems
struct LoreEvent {
  int year, month, day;
  std::string category;
  int cellID;
  std::string log;
};

class LoreScribe {
private:
  std::ofstream historyFile;
  std::vector<LoreEvent> sessionLogs;

public:
  void Initialize(std::string worldName);
  void RecordEvent(const ChronosConfig &time, std::string cat, int cellID,
                   std::string text);
  const std::vector<LoreEvent> &GetSessionLogs();
};

// Timelapse System
struct Snapshot {
  int year;
  std::vector<int> factionData;
  std::vector<float> popData;
};

class TimelapseManager {
public:
  uint32_t taggedCellStart;
  int radius;
  std::vector<Snapshot> history;

  void TagRegion(uint32_t cellID, int r);
  void TakeSnapshot(const WorldBuffers &b, const ChronosConfig &time);
};

// Logistics Function Prototype
void ProcessLogistics(WorldBuffers &buffers,
                      const std::vector<Faction> &factions, uint32_t count,
                      int resourceID);

class ConflictSystem {
public:
  void ResolveBorders(WorldBuffers &b,
                      const std::vector<AgentTemplate> &registry,
                      const std::vector<Faction> &factions,
                      const NeighborGraph &graph, LoreScribe &scribe,
                      const ChronosConfig &time, float deltaTime);
};

// Unified Simulation Loop Logic
class SimulationLoop {
public:
  AgentSystem agentSystem;
  ConflictSystem conflictSystem;
  WorldSettings settings;
  LoreScribe scribe;
  TimelapseManager timelapse;

  void Update(WorldBuffers &buffers, std::vector<AgentTemplate> &registry,
              std::vector<Faction> &factions, ChronosConfig &time,
              const NeighborGraph &graph, float dt);
};

class ChronosSystem {
public:
  ClimateSim *climate;
  AgentSystem *agents;
  WorldSettings *settings;
  std::vector<AgentTemplate> *registry;

  void GlobalChronosTick(WorldBuffers &buffers, ChronosConfig &time,
                         float realDeltaTime);

private:
  void UpdateWeatherSystems(WorldBuffers &buffers);
  void ProcessAgentMovement(WorldBuffers &buffers);
  void RegenerateResources(WorldBuffers &buffers);
  void ProcessRandomEvents();
};

// Point Generation
void GeneratePoisson(WorldBuffers &buffers, WorldSettings &settings);
// Heightmap
void LoadHeightmapData(const char *path, WorldBuffers &buffers, uint32_t count);
// Chaos Simulation
void TickChaos(WorldBuffers &buffers, NeighborGraph &graph,
               float diffusionRate);
