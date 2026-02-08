#pragma once
#include "WorldEngine.hpp"

// AgentSystem (src/biology/AgentSystem.cpp)
namespace AgentSystem {
extern std::vector<AgentTemplate> speciesRegistry;
void Initialize();
void UpdateBiology(WorldBuffers &b, const WorldSettings &s);
void SpawnCivilization(WorldBuffers &b, int factionId);
void UpdateCivilization(WorldBuffers &b, const NeighborGraph &g);
} // namespace AgentSystem
