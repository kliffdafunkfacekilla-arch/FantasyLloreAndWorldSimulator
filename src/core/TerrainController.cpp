#include "../../include/SimulationModules.hpp"
#include "../../include/FastNoiseLite.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

void TerrainController::GenerateProceduralTerrain(WorldBuffers& buffers, const WorldSettings& settings) {
  if (settings.heightmapPath[0] != '\0') {
    std::cout << "[TERRAIN] Heightmap path set: " << settings.heightmapPath << ". Use 'Import Heightmap' button to load.\n";
  }

  std::cout << "[TERRAIN] Generating procedural terrain with Seed "
            << settings.seed << "\n";

  FastNoiseLite noise;
  noise.SetSeed(settings.seed);
  noise.SetFrequency(settings.featureFrequency);
  noise.SetFractalLacunarity(settings.featureClustering);
  noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
  noise.SetFractalType(FastNoiseLite::FractalType_FBm);
  noise.SetFractalOctaves(4);

  int side = (int)std::sqrt(settings.cellCount);
  if (side == 0) side = 1000; // Safety

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

    if (buffers.height)
        buffers.height[i] = h;
  }
}

void TerrainController::LoadFromImage(const char* path, WorldBuffers& buffers) {
  if (!path || path[0] == '\0')
    return;
  std::cout << "[TERRAIN] Importing heightmap from: " << path << std::endl;
  LoadHeightmapData(path, buffers, buffers.count);
}

void TerrainController::ApplyThermalErosion(WorldBuffers& buffers, int iterations) {
    // Simplified erosion (Smoothing)
    // Assumes grid layout for simplicity as we don't have NeighborGraph passed here yet

    std::cout << "[TERRAIN] Apply Thermal Erosion (" << iterations << " passes)...\n";

    int side = (int)std::sqrt(buffers.count);
    if (side == 0) return;

    // Scratch buffer
    std::vector<float> tempHeight(buffers.count);

    for (int iter = 0; iter < iterations; ++iter) {
        for (uint32_t i = 0; i < buffers.count; ++i) {
            float sum = buffers.height[i];
            int count = 1;

            // Grid neighbors
            int neighbors[] = { -side, side, -1, 1 };
            for(int n : neighbors) {
                int idx = (int)i + n;
                // Bounds check
                if (idx >= 0 && idx < (int)buffers.count) {
                    // Check wrap-around for left/right
                    if (n == -1 && i % side == 0) continue;
                    if (n == 1 && (i + 1) % side == 0) continue;

                    sum += buffers.height[idx];
                    count++;
                }
            }

            tempHeight[i] = sum / count;
        }

        // Write back
        for (uint32_t i = 0; i < buffers.count; ++i) {
            buffers.height[i] = tempHeight[i];
        }
    }
}
