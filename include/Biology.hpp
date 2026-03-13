#pragma once
#include "WorldEngine.hpp"

// AgentSystem (src/biology/AgentSystem.cpp)
namespace AgentSystem {
extern std::vector<AgentTemplate> speciesRegistry;
void Initialize();
void UpdateBiology(WorldBuffers &b, const NeighborGraph &g,
                   const WorldSettings &s, const ChronosConfig &c);
void SpawnLife(WorldBuffers &b, int count);
void SpawnCivilization(WorldBuffers &b, int count);
void UpdateCivilization(WorldBuffers &b, const NeighborGraph &g);
} // namespace AgentSystem
