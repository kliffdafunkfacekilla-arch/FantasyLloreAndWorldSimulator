#include "../../include/SimulationModules.hpp"
#include "../core/FastNoiseLite.h"
#include <algorithm>
#include <cmath>
#include <cstring> // For memset etc if needed
#include <iostream>


void TerrainController::GenerateTerrain(WorldBuffers &buffers,
                                        const WorldSettings &settings) {
  if (settings.heightmapPath[0] != '\0') {
    std::cout << "[TERRAIN] Checkbox to load custom map? Or checking if path "
                 "is valid...\n";
    // Logic handled by explicit "Load" button usually
  }

  std::cout << "[TERRAIN] Generating procedural terrain with Seed "
            << settings.seed << "\n";
  std::cout << "Params: Freq=" << settings.featureFrequency
            << ", Lacunarity=" << settings.featureClustering
            << ", Severity=" << settings.heightSeverity << "\n";

  FastNoiseLite noise;
  noise.SetSeed(settings.seed);
  noise.SetFrequency(settings.featureFrequency);
  noise.SetFractalLacunarity(settings.featureClustering);
  noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
  noise.SetFractalType(FastNoiseLite::FractalType_FBm);
  noise.SetFractalOctaves(4);

  int side = (int)std::sqrt(settings.cellCount);
  for (uint32_t i = 0; i < settings.cellCount; ++i) {
    if (buffers.posX && buffers.posY) {
      buffers.posX[i] = (float)(i % side) / side;
      buffers.posY[i] = (float)(i / side) / side;
    }
  }

  for (uint32_t i = 0; i < settings.cellCount; ++i) {
    float h =
        noise.GetNoise(buffers.posX[i] * 100.0f, buffers.posY[i] * 100.0f);
    h = (h + 1.0f) * 0.5f;

    if (h > 0)
      h = std::pow(h, settings.heightSeverity);
    h *= settings.heightMultiplier;

    h = std::clamp(h, settings.heightMin, settings.heightMax);
    h = std::clamp(h, 0.0f, 1.0f);

    buffers.height[i] = h;
  }
}

void TerrainController::RunThermalErosion(WorldBuffers &buffers,
                                          const NeighborGraph &graph,
                                          int iterations, uint32_t count) {
  float talusThreshold = 0.01f;
  float moveAmount = 0.005f;

  for (int iter = 0; iter < iterations; ++iter) {
    for (uint32_t i = 0; i < count; ++i) {
      int offset = graph.offsetTable[i];
      uint8_t n_count = graph.countTable[i];

      for (int n = 0; n < n_count; ++n) {
        int neighborID = graph.neighborData[offset + n];
        float diff = buffers.height[i] - buffers.height[neighborID];

        if (diff > talusThreshold) {
          buffers.height[i] -= moveAmount;
          buffers.height[neighborID] += moveAmount;
        }
      }
    }
  }
  std::cout << "[TERRAIN] Erosion pass.\n";
}

void TerrainController::LoadFromImage(const char *path, WorldBuffers &buffers) {
  if (!path || path[0] == '\0')
    return;
  std::cout << "[TERRAIN] Importing heightmap from: " << path << std::endl;
  // Call the existing helper function declared in SimulationModules.hpp
  LoadHeightmapData(path, buffers, buffers.count);
}

void TerrainController::ApplyThermalErosion(WorldBuffers &buffers,
                                            const NeighborGraph &graph) {
  RunThermalErosion(buffers, graph, 1, buffers.count);
}
