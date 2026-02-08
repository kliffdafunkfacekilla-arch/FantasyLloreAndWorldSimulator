#pragma once
#include "WorldEngine.hpp"

// Climate Engine (src/environment/ClimateSim.cpp)
namespace ClimateSim {
void Update(WorldBuffers &b, const WorldSettings &s);
}

// Hydrology (src/environment/HydrologySim.cpp)
namespace HydrologySim {
void Update(WorldBuffers &b, const NeighborGraph &g, const WorldSettings &s);
}

// ChaosField namespace (src/environment/ChaosField.cpp)
namespace ChaosField {
void SpawnRift(WorldBuffers &b, int index, float intensity);
void ClearRifts();
void Update(WorldBuffers &b, const NeighborGraph &g);
} // namespace ChaosField

// Disaster System (src/environment/DisasterSystem.cpp)
namespace DisasterSystem {
void Update(WorldBuffers &b, const WorldSettings &s);
void Trigger(WorldBuffers &b, int type, int index, float strength);
} // namespace DisasterSystem

// ChaosField (Free function - legacy wrapper)
void UpdateChaos(WorldBuffers &b, const NeighborGraph &graph,
                 float diffusionRate);
