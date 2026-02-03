#include "../../include/SimulationModules.hpp"
#include "../core/FastNoiseLite.h"
#include <algorithm>
#include <cmath>
#include <iostream>


void TerrainController::GenerateTerrain(WorldBuffers &buffers,
                                        const WorldSettings &settings) {
  if (!settings.heightmapPath.empty()) {
    std::cout << "[TERRAIN] Loading heightmap from " << settings.heightmapPath
              << "\n";
    LoadHeightmapData(settings.heightmapPath.c_str(), buffers,
                      settings.cellCount);
    return;
  }

  std::cout << "[TERRAIN] Generating procedural terrain with Seed "
            << settings.seed << "\n";
  std::cout << "Params: Freq=" << settings.featureFrequency
            << ", Lacunarity=" << settings.featureClustering
            << ", Severity=" << settings.heightSeverity << "\n";

  // We use a noise library like FastNoiseLite for the high spots
  FastNoiseLite noise;
  noise.SetSeed(settings.seed);
  noise.SetFrequency(settings.featureFrequency);          // User slider
  noise.SetFractalLacunarity(settings.featureClustering); // User slider

  // Using Fractal Noise for better "clustering" effect if Lacunarity is used
  noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
  noise.SetFractalType(FastNoiseLite::FractalType_FBm);
  noise.SetFractalOctaves(4);

  // Initialize Grid Coordinates
  int side = (int)std::sqrt(settings.cellCount);
  for (uint32_t i = 0; i < settings.cellCount; ++i) {
    if (buffers.posX && buffers.posY) {
      buffers.posX[i] = (float)(i % side) / side;
      buffers.posY[i] = (float)(i / side) / side;
    }
  }

  for (uint32_t i = 0; i < settings.cellCount; ++i) {
    // 1. Get Base Noise
    // Scale coords significantly to get features
    float h =
        noise.GetNoise(buffers.posX[i] * 100.0f, buffers.posY[i] * 100.0f);

    // Normalize noise from -1..1 to 0..1
    h = (h + 1.0f) * 0.5f;

    // 2. Adjust Severity (The Power Function)
    // h^k: if k>1, low values drop faster (more plains, sharper peaks)
    // if k<1, values boost (more plateaus)
    if (h > 0)
      h = std::pow(h, settings.heightSeverity);

    // 3. Apply Multiplier and Min/Max Clamping
    h *= settings.heightMultiplier;

    // Clamp logic: Ensure it fits within user range, but also 0..1 global
    h = std::clamp(h, settings.heightMin, settings.heightMax);
    h = std::clamp(h, 0.0f, 1.0f); // Safety clamp

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
  std::cout << "[TERRAIN] Erosion pass complete (" << iterations
            << " iterations).\n";
}
