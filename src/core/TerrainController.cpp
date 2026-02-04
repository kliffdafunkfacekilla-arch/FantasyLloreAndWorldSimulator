#include "../../include/FastNoiseLite.h"
#include "../../include/SimulationModules.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

void TerrainController::GenerateProceduralTerrain(
    WorldBuffers &buffers, const WorldSettings &settings) {
  if (settings.heightmapPath[0] != '\0') {
    std::cout << "[TERRAIN] Heightmap path set: " << settings.heightmapPath
              << ". Use 'Import Heightmap' button to load.\n";
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
  if (side == 0)
    side = 1000; // Safety

  for (uint32_t i = 0; i < settings.cellCount; ++i) {
    if (buffers.posX && buffers.posY) {
      buffers.posX[i] = (float)(i % side) / side;
      buffers.posY[i] = (float)(i / side) / side;
      if (i < 5) {
        std::cout << "[TERRAIN] Generated pos[" << i << "]: ("
                  << buffers.posX[i] << ", " << buffers.posY[i] << ")\n";
      }
    }
  }

  for (uint32_t i = 0; i < settings.cellCount; ++i) {
    // 1. Get Base Noise (0.0 to 1.0)
    // Range -1..1 -> 0..1
    float h =
        noise.GetNoise(buffers.posX[i] * 100.0f, buffers.posY[i] * 100.0f);
    h = (h + 1.0f) * 0.5f;

    // 2. Shape it (Severity makes valleys wider/peaks thinner)
    if (h > 0)
      h = std::pow(h, settings.heightSeverity);

    // 3. Apply Range (Map 0.0-1.0 to Min-Max)
    // This preserves detail instead of clipping it
    float range = settings.heightMax - settings.heightMin;
    h = settings.heightMin + (h * range);

    // 4. Finally apply the multiplier (if you really want to scale it up)
    h *= settings.heightMultiplier;

    // Soft clamp to valid range (0-1) for rendering
    h = std::max(0.0f, std::min(h, 1.0f));

    if (buffers.height)
      buffers.height[i] = h;

    // Basic Temperature Generation (Latitude + Height)
    if (buffers.temperature) {
      // Temperature drops as you go from Equator (Y=0.5) to Poles (Y=0.0/1.0)
      // Simple linear gradient for now
      float latitude = std::abs(buffers.posY[i] - 0.5f) *
                       2.0f; // 0.0 at equator, 1.0 at poles
      float temp = 1.0f - latitude;

      // Height cooling (lapse rate)
      temp -= h * 0.5f;

      buffers.temperature[i] = std::clamp(temp, 0.0f, 1.0f);
    }

    // Basic Moisture (Noise)
    if (buffers.moisture) {
      float m = noise.GetNoise(buffers.posX[i] * 50.0f + 1000.0f,
                               buffers.posY[i] * 50.0f + 1000.0f);
      m = (m + 1.0f) * 0.5f;
      buffers.moisture[i] = m;
    }
  }
}

void TerrainController::LoadFromImage(const char *path, WorldBuffers &buffers) {
  if (!path || path[0] == '\0')
    return;
  std::cout << "[TERRAIN] Importing heightmap from: " << path << std::endl;
  LoadHeightmapData(path, buffers, buffers.count);
}

void TerrainController::ApplyThermalErosion(WorldBuffers &buffers,
                                            int iterations) {
  // Simplified erosion (Smoothing)
  // Assumes grid layout for simplicity as we don't have NeighborGraph passed
  // here yet

  std::cout << "[TERRAIN] Apply Thermal Erosion (" << iterations
            << " passes)...\n";

  int side = (int)std::sqrt(buffers.count);
  if (side == 0)
    return;

  // Talus Threshold: Material only moves if diff > 4 / side
  // This prevents infinite slumping on flat-ish ground
  float talus = 4.0f / side;

  for (int iter = 0; iter < iterations; ++iter) {
    for (uint32_t i = 0; i < buffers.count; ++i) {
      float h = buffers.height[i];

      int neighbors[] = {-side, side, -1, 1};
      for (int n : neighbors) {
        int idx = (int)i + n;

        // Bounds check
        if (idx >= 0 && idx < (int)buffers.count) {
          // Basic wrap check
          if (n == -1 && i % side == 0)
            continue;
          if (n == 1 && (i + 1) % side == 0)
            continue;

          float hN = buffers.height[idx];
          float diff = h - hN;

          if (diff > talus) {
            float transfer = diff * 0.5f; // Move half the excess
            buffers.height[i] -= transfer;
            buffers.height[idx] += transfer;
            // Note: This is in-place mass transfer (unstable but organic)
          }
        }
      }
    }
  }
}
