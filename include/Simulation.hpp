#pragma once
#include "WorldEngine.hpp"

// Main Simulation Loop (src/simulation/SimulationLoop.cpp)
class SimulationLoop {
public:
  void Update(WorldBuffers &b, ChronosConfig &clock, const WorldSettings &s);
};

// CivilizationSim namespace (src/simulation/CivilizationSim.cpp)
namespace CivilizationSim {
void Update(WorldBuffers &b, const NeighborGraph &g, const WorldSettings &s);
void EvolveSettlements(WorldBuffers &b);
void ConstructBuildings(WorldBuffers &b);
void ProduceResources(WorldBuffers &b);
} // namespace CivilizationSim

// ConflictSystem (src/simulation/ConflictSystem.cpp)
namespace ConflictSystem {
void Update(WorldBuffers &b, const NeighborGraph &g, const WorldSettings &s);
}

// LogisticsSystem namespace (src/simulation/LogisticsSystem.cpp)
namespace LogisticsSystem {
void Update(WorldBuffers &b, const NeighborGraph &g);
}

// UnitSystem namespace (src/simulation/UnitSystem.cpp)
namespace UnitSystem {
void Update(WorldBuffers &b, int mapWidth);
void MoveUnits(WorldBuffers &b, int mapWidth);
void DeliverCargo(WorldBuffers &b, int mapWidth);
void ResolveCombat(WorldBuffers &b, int mapWidth);
} // namespace UnitSystem

// NeighborFinder (src/simulation/NeighborFinder.cpp) -- Often used in sim
class NeighborFinder {
public:
  void BuildGraph(WorldBuffers &buffers, uint32_t count, NeighborGraph &graph);
  void Cleanup(NeighborGraph &graph);
};

// Legacy Wrappers
void ResolveConflicts(WorldBuffers &b, const NeighborGraph &graph);
void ProcessLogistics(WorldBuffers &b, uint32_t count);
