#include "../../include/FastNoiseLite.h"
#include "../../include/Terrain.hpp"
#include "../../include/stb_image.h"
#include <cmath>
#include <vector>

// Helper clamp since std::clamp can be tricky in some MinGW versions
template <typename T> T clamp_val(T val, T min, T max) {
  if (val < min)
    return min;
  if (val > max)
    return max;
  return val;
}

// Forward declaration for HeightmapLoader logic
void LoadHeightmapData(const char *path, WorldBuffers &buffers, uint32_t count);

void TerrainController::GenerateHeightmap(WorldBuffers &b,
                                          const WorldSettings &s) {
  FastNoiseLite noise;
  noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
  noise.SetFrequency(0.005f);
  noise.SetFractalType(FastNoiseLite::FractalType_FBm);
  noise.SetFractalOctaves(5);
  noise.SetSeed(s.seed);

  int side = (int)std::sqrt(b.count);
  for (int i = 0; i < (int)b.count; ++i) {
    int x = i % side;
    int y = i / side;

    float n = noise.GetNoise((float)x, (float)y);
    b.height[i] = (n * 0.5f + 0.5f);

    if (s.islandMode) {
      float dx = (x - side / 2.0f) / (side / 2.0f);
      float dy = (y - side / 2.0f) / (side / 2.0f);
      float dist = std::sqrt(dx * dx + dy * dy);
      float mask = 1.0f - std::pow(dist, 1.5f);
      if (mask < 0)
        mask = 0;
      if (mask > 1)
        mask = 1;
      b.height[i] *= mask;
    }

    if (b.height[i] < 0)
      b.height[i] = 0;
  }
}

void TerrainController::GenerateTectonicPlates(WorldBuffers &b,
                                               const WorldSettings &s) {
  int side = (int)std::sqrt(b.count);
  if (side <= 0)
    return;

  FastNoiseLite plateNoise;
  plateNoise.SetNoiseType(FastNoiseLite::NoiseType_Cellular);
  plateNoise.SetCellularDistanceFunction(
      FastNoiseLite::CellularDistanceFunction_Euclidean);
  plateNoise.SetCellularReturnType(FastNoiseLite::CellularReturnType_CellValue);
  plateNoise.SetFrequency(0.002f);
  plateNoise.SetSeed(s.seed);

  FastNoiseLite edgeNoise;
  edgeNoise.SetNoiseType(FastNoiseLite::NoiseType_Cellular);
  edgeNoise.SetCellularReturnType(FastNoiseLite::CellularReturnType_Distance2);
  edgeNoise.SetFrequency(0.002f);
  edgeNoise.SetSeed(s.seed);

  FastNoiseLite detail;
  detail.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
  detail.SetFrequency(0.02f);

  for (int i = 0; i < (int)b.count; ++i) {
    int x = i % side;
    int y = i / side;

    float plateID = plateNoise.GetNoise((float)x, (float)y);
    float plateHeight = std::abs(plateID);
    float distToEdge = (edgeNoise.GetNoise((float)x, (float)y) + 1.0f) * 0.5f;

    float finalHeight;
    if (plateHeight < 0.4f) {
      finalHeight = 0.1f + (detail.GetNoise((float)x, (float)y) * 0.05f);
      if (distToEdge < 0.1f)
        finalHeight -= 0.2f;
    } else {
      finalHeight = 0.5f + (plateHeight * 0.2f);
      if (distToEdge < 0.15f) {
        float mountainFactor = 1.0f - (distToEdge / 0.15f);
        finalHeight += mountainFactor * 0.6f;
      }
      finalHeight += detail.GetNoise((float)x, (float)y) * 0.1f;
    }
    b.height[i] = clamp_val(finalHeight, 0.0f, 1.0f);
  }
}

void TerrainController::ApplyBrush(WorldBuffers &b, int width, int cx, int cy,
                                   float r, float str, int mode) {
  int side = (int)std::sqrt(b.count);
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
        float sum = 0;
        int count = 0;
        for (int ny = y - 1; ny <= y + 1; ++ny) {
          for (int nx = x - 1; nx <= x + 1; ++nx) {
            if (nx >= 0 && nx < side && ny >= 0 && ny < side) {
              sum += b.height[ny * side + nx];
              count++;
            }
          }
        }
        b.height[idx] = (b.height[idx] * (1.0f - str * falloff)) +
                        (sum / (float)count * str * falloff);
      }
      b.height[idx] = clamp_val(b.height[idx], 0.0f, 1.0f);
    }
  }
}

void TerrainController::LoadHeightmapFromImage(WorldBuffers &b,
                                               const std::string &filepath) {
  LoadHeightmapData(filepath.c_str(), b, b.count);
}

void TerrainController::ApplyThermalErosion(WorldBuffers &b, int iterations) {
  int side = (int)std::sqrt(b.count);
  float threshold = 0.01f;
  float amount = 0.1f;

  for (int iter = 0; iter < iterations; ++iter) {
    for (int i = 0; i < (int)b.count; ++i) {
      float h = b.height[i];
      int x = i % side;
      int y = i / side;

      int neighbors[] = {i - 1, i + 1, i - side, i + side};
      for (int n : neighbors) {
        if (n >= 0 && n < (int)b.count) {
          // Spatial continuity check for horizontal neighbors
          if ((n == i - 1 || n == i + 1) && (n / side != y))
            continue;

          float dh = h - b.height[n];
          if (dh > threshold) {
            float move = (dh - threshold) * amount;
            b.height[i] -= move;
            b.height[n] += move;
          }
        }
      }
    }
  }
}

void TerrainController::EnforceOceanEdges(WorldBuffers &b, int side,
                                          float fadeDist) {
  for (int i = 0; i < (int)b.count; ++i) {
    int x = i % side;
    int y = i / side;
    float dx = (float)(x - side / 2) / ((float)side / 2.0f);
    float dy = (float)(y - side / 2) / ((float)side / 2.0f);
    float dist = std::sqrt(dx * dx + dy * dy);
    float mask = (fadeDist > 0.0001f) ? (1.0f - dist) / fadeDist
                                      : (dist < 1.0f ? 1.0f : 0.0f);
    if (mask < 0)
      mask = 0;
    if (mask > 1)
      mask = 1;
    b.height[i] *= mask;
  }
}

void TerrainController::SmoothTerrain(WorldBuffers &b, int side) {
  std::vector<float> nextH(b.count);
  for (int i = 0; i < (int)b.count; ++i) {
    int x = i % side;
    int y = i / side;
    float sum = 0;
    int count = 0;
    for (int ny = y - 1; ny <= y + 1; ++ny) {
      for (int nx = x - 1; nx <= x + 1; ++nx) {
        if (nx >= 0 && nx < side && ny >= 0 && ny < side) {
          sum += b.height[ny * side + nx];
          count++;
        }
      }
    }
    nextH[i] = sum / (float)count;
  }
  for (int i = 0; i < (int)b.count; ++i)
    b.height[i] = nextH[i];
}

void TerrainController::RoughenCoastlines(WorldBuffers &b, int side,
                                          float seaLevel) {
  FastNoiseLite noise;
  noise.SetFrequency(0.05f);
  for (int i = 0; i < (int)b.count; ++i) {
    if (b.height[i] > seaLevel - 0.05f && b.height[i] < seaLevel + 0.05f) {
      b.height[i] +=
          noise.GetNoise((float)(i % side), (float)(i / side)) * 0.02f;
    }
    b.height[i] = clamp_val(b.height[i], 0.0f, 1.0f);
  }
}

void TerrainController::GenerateClimate(WorldBuffers &b,
                                        const WorldSettings &s) {}
void TerrainController::SimulateHydrology(WorldBuffers &b,
                                          const WorldSettings &s) {}
void TerrainController::RecalculateBiomes(WorldBuffers &b) {}
