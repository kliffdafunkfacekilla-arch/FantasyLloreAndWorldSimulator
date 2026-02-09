#include "../../include/FastNoiseLite.h"
#include "../../include/Terrain.hpp"
#include "../../include/stb_image.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

void TerrainController::GenerateHeightmap(WorldBuffers &b,
                                          const WorldSettings &s) {
  FastNoiseLite noise;
  noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
  noise.SetFrequency(0.005f); // Use settings.noiseFreq if available
  noise.SetFractalType(FastNoiseLite::FractalType_FBm);
  noise.SetFractalOctaves(5);

  int side = std::sqrt(b.count);
  for (int i = 0; i < b.count; ++i) {
    int x = i % side;
    int y = i / side;

    // Base Noise
    float n = noise.GetNoise((float)x, (float)y);

    // Island Mask (Simple circular falloff)
    float dx = (x - side / 2.0f) / (side / 2.0f);
    float dy = (y - side / 2.0f) / (side / 2.0f);
    float dist = std::sqrt(dx * dx + dy * dy);
    float mask = 1.0f - std::pow(dist, 1.5f); // Tune exponent for island size

    b.height[i] = (n * 0.5f + 0.5f) * mask;
    if (b.height[i] < 0)
      b.height[i] = 0;
  }
}

void TerrainController::GenerateClimate(WorldBuffers &b,
                                        const WorldSettings &s) {
  int side = std::sqrt(b.count);
  FastNoiseLite mNoise;
  mNoise.SetFrequency(0.003f);

  for (int i = 0; i < b.count; ++i) {
    int x = i % side;
    int y = i / side;

    // 1. TEMPERATURE (Based on Latitude/Y + Height)
    float latitude = (float)y / side; // 0.0 (North) to 1.0 (South)
    float baseTemp = 1.0f - std::abs(latitude - 0.5f) * 2.0f; // Equator hot
    // Altitude Penalty (Higher = Colder)
    baseTemp -= std::max(0.0f, b.height[i] - s.seaLevel) * 0.5f;

    float finalT = baseTemp;
    if (finalT < 0.0f)
      finalT = 0.0f;
    if (finalT > 1.0f)
      finalT = 1.0f;
    b.temperature[i] = finalT;

    // 2. MOISTURE (Perlin Noise + Proximity to Water)
    float m = mNoise.GetNoise((float)x, (float)y) * 0.5f + 0.5f;
    if (b.height[i] < s.seaLevel)
      m = 1.0f; // Ocean is wet
    b.moisture[i] = m;
  }
}

void TerrainController::SimulateHydrology(WorldBuffers &b,
                                          const WorldSettings &s) {
  // Simple hydraulic erosion / river mapping
  // Reset moisture runoff
  std::vector<float> flow(b.count, 0.0f);
  int side = std::sqrt(b.count);

  // Sort cells by height (highest to lowest)
  struct Cell {
    int id;
    float h;
  };
  std::vector<Cell> cells(b.count);
  for (int i = 0; i < b.count; ++i)
    cells[i] = {i, b.height[i]};

  std::sort(cells.begin(), cells.end(),
            [](const Cell &a, const Cell &b) { return a.h > b.h; });

  for (const auto &c : cells) {
    int i = c.id;
    // Rain falls here based on moisture
    flow[i] += b.moisture[i];

    // Find lowest neighbor
    int bestN = -1;
    float minH = b.height[i];
    int x = i % side;
    int y = i / side;

    int nIndices[] = {i - 1, i + 1, i - side, i + side};
    for (int nIdx : nIndices) {
      if (nIdx >= 0 && nIdx < b.count && b.height[nIdx] < minH) {
        minH = b.height[nIdx];
        bestN = nIdx;
      }
    }

    // Pass water down
    if (bestN != -1) {
      flow[bestN] += flow[i];

      // If massive flow, carve river
      if (flow[i] > 10.0f && b.height[i] > s.seaLevel) {
        // b.height[i] -= 0.005f; // Optional Erosion
        b.moisture[i] += 0.5f; // River adds moisture
      }
    }
  }
}

void TerrainController::ApplyBrush(WorldBuffers &b, int width, int cx, int cy,
                                   float r, float str, int mode) {
  int side = std::sqrt(b.count);
  int rInt = (int)r;
  for (int y = cy - rInt; y <= cy + rInt; ++y) {
    if (y < 0 || y >= side)
      continue;
    for (int x = cx - rInt; x <= cx + rInt; ++x) {
      if (x < 0 || x >= side)
        continue;
      int idx = y * side + x;
      float dist = std::sqrt((x - cx) * (x - cx) + (y - cy) * (y - cy));
      if (dist > r)
        continue;

      float falloff = 1.0f - (dist / r);
      if (mode == 0)
        b.height[idx] += str * falloff;
      else if (mode == 1)
        b.height[idx] -= str * falloff;
      else if (mode == 2) {
        // Smoothing placeholder
      }
      if (b.height[idx] < 0)
        b.height[idx] = 0;
      if (b.height[idx] > 1.0f)
        b.height[idx] = 1.0f;
    }
  }
}

void TerrainController::LoadHeightmapFromImage(WorldBuffers &b,
                                               const std::string &filepath) {
  int w, h, channels;
  unsigned char *data = stbi_load(filepath.c_str(), &w, &h, &channels, 1);
  if (!data)
    return;

  int side = std::sqrt(b.count);
  for (int y = 0; y < side; ++y) {
    for (int x = 0; x < side; ++x) {
      int imgX = (x * w) / side;
      int imgY = (y * h) / side;
      int idx = imgY * w + imgX;
      b.height[y * side + x] = data[idx] / 255.0f;
    }
  }
  stbi_image_free(data);
}

void TerrainController::RecalculateBiomes(WorldBuffers &b) {
  // Biome calculation based on Whittaker diagram (Temp vs Moisture)
  for (int i = 0; i < (int)b.count; i++) {
    // Placeholder for biome logic
  }
}

void TerrainController::ApplyThermalErosion(WorldBuffers &b, int iterations) {
  // Stub
}

void TerrainController::EnforceOceanEdges(WorldBuffers &b, int side,
                                          float fadeDist) {
  // Stub
}

void TerrainController::SmoothTerrain(WorldBuffers &b, int side) {
  // Stub
}

void TerrainController::RoughenCoastlines(WorldBuffers &b, int side,
                                          float seaLevel) {
  // Stub
}
