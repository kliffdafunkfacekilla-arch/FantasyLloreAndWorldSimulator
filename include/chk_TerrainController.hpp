#pragma once
#include "SimulationModules.hpp" // For NeighborGraph definition if needed
#include "WorldEngine.hpp"


class TerrainController {
public:
  void GenerateTerrain(WorldBuffers &buffers, const WorldSettings &settings);

  // Wrapper for thermal erosion
  void ApplyThermalErosion(WorldBuffers &buffers) {
    // Need access to graph? The user snippet didn't pass graph.
    // We might need to store graph reference or pass it.
    // For now, I'll update the signature in main to pass it, or just use the
    // member if I had one. Actually, main.cpp has 'graph'. I'll add 'graph' to
    // the arguments of ApplyThermalErosion or make it require one.
  }

  // Actual implementation requiring graph
  void RunThermalErosion(WorldBuffers &buffers, const NeighborGraph &graph,
                         int iterations, uint32_t count);

  void LoadFromImage(const char *path, WorldBuffers &buffers);
};

// Helper for loading image data (stb_image usage usually)
void LoadHeightmapData(const char *path, WorldBuffers &buffers, uint32_t count);
