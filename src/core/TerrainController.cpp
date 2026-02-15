#include "../../include/FastNoiseLite.h"
#include "../../include/Terrain.hpp"
#include "../../include/stb_image.h"
#include <cmath>
#include <iostream>
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
  noise.SetSeed(s.seed);
  noise.SetFractalType(FastNoiseLite::FractalType_FBm);
  noise.SetFractalOctaves(5);

  int side = (int)std::sqrt(b.count);
  float freq = 0.005f;

  // Branch based on Template
  switch (s.worldType) {
  case TEMPLATE_CONTINENTS:
    noise.SetFrequency(0.004f);
    noise.SetFractalOctaves(4);
    break;
  case TEMPLATE_ISLAND_CHAIN:
    noise.SetFrequency(0.02f);
    break;
  case TEMPLATE_SINGLE_LANDMASS:
    noise.SetFrequency(0.006f);
    break;
  case TEMPLATE_TWIN_LANMASSES:
    noise.SetFrequency(0.005f);
    break;
  case TEMPLATE_BROKEN:
    noise.SetFrequency(0.04f);
    noise.SetFractalOctaves(6);
    break;
  default:
    noise.SetFrequency(0.005f);
    break;
  }

  for (int i = 0; i < (int)b.count; ++i) {
    int x = i % side;
    int y = i / side;

    float n = noise.GetNoise((float)x, (float)y);
    float h = (n * 0.5f + 0.5f);

    // Apply Template-specific masks/distortions
    if (s.worldType == TEMPLATE_SINGLE_LANDMASS || s.islandMode) {
      float dx = (x - side / 2.0f) / (side / 2.0f);
      float dy = (y - side / 2.0f) / (side / 2.0f);
      float dist = std::sqrt(dx * dx + dy * dy);
      float mask = 1.0f - std::pow(dist, 1.5f);
      h *= clamp_val(mask, 0.0f, 1.0f);
    } else if (s.worldType == TEMPLATE_TWIN_LANMASSES) {
      // Two blobs
      float dx1 = (x - side * 0.3f) / (side / 3.0f);
      float dy1 = (y - side * 0.5f) / (side / 3.0f);
      float dx2 = (x - side * 0.7f) / (side / 3.0f);
      float dy2 = (y - side * 0.5f) / (side / 3.0f);
      float d1 = std::sqrt(dx1 * dx1 + dy1 * dy1);
      float d2 = std::sqrt(dx2 * dx2 + dy2 * dy2);
      float mask = (1.0f - std::pow(d1, 2.0f)) + (1.0f - std::pow(d2, 2.0f));
      h *= clamp_val(mask, 0.0f, 1.0f);
    } else if (s.worldType == TEMPLATE_CONTINENTS) {
      // Tectonic favor: slightly boost center areas
      float dx = (x - side / 2.0f) / (side / 2.0f);
      float dy = (y - side / 2.0f) / (side / 2.0f);
      float dist = std::sqrt(dx * dx + dy * dy);
      if (dist > 0.8f)
        h *= (1.0f - (dist - 0.8f) * 5.0f);
    }

    b.height[i] = clamp_val(h, 0.0f, 1.0f);
  }
}

void TerrainController::GenerateTectonicPlates(WorldBuffers &b,
                                               const WorldSettings &s) {
  std::cout << "[DEBUG] Generating Tectonic Plates..." << std::endl;
  int side = (int)std::sqrt(b.count);
  if (side <= 0)
    return;

  // 1. Primary Plates (Large Continents)
  FastNoiseLite continentNoise;
  continentNoise.SetNoiseType(FastNoiseLite::NoiseType_Cellular);
  continentNoise.SetCellularDistanceFunction(
      FastNoiseLite::CellularDistanceFunction_Euclidean);
  continentNoise.SetCellularReturnType(
      FastNoiseLite::CellularReturnType_Distance2); // Distance to edge
  continentNoise.SetFrequency(0.003f);              // Large plates
  continentNoise.SetSeed(s.seed);
  continentNoise.SetCellularJitter(1.2f); // Organic shapes

  // 2. Secondary Plates (Breakup)
  FastNoiseLite breakupNoise;
  breakupNoise.SetNoiseType(FastNoiseLite::NoiseType_Cellular);
  breakupNoise.SetCellularReturnType(
      FastNoiseLite::CellularReturnType_CellValue);
  breakupNoise.SetFrequency(0.01f);
  breakupNoise.SetSeed(s.seed + 1);

  // 3. Detail Noise (Roughness)
  FastNoiseLite detail;
  detail.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
  detail.SetFrequency(0.02f);
  detail.SetFractalType(FastNoiseLite::FractalType_FBm);
  detail.SetFractalOctaves(3);

  // 4. Warp Noise (Distortion)
  FastNoiseLite warp;
  warp.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
  warp.SetFrequency(0.005f);

  for (int i = 0; i < (int)b.count; ++i) {
    int x = i % side;
    int y = i / side;

    float wx = (float)x;
    float wy = (float)y;
    float wForce = 40.0f;
    float wn = warp.GetNoise(wx, wy);
    wx += wn * wForce;
    wy += wn * wForce;

    // Plate math
    float distToEdge = continentNoise.GetNoise(wx, wy); // -1 to 1 range usually
    distToEdge = (distToEdge + 1.0f) * 0.5f;            // 0 to 1

    // Invert: 1.0 is center of plate, 0.0 is edge
    float plateHeight = distToEdge;

    // Add breakup
    float breakup = breakupNoise.GetNoise(wx, wy);

    // Continental Shelf Logic
    float finalHeight = 0.0f;

    // If we are "on a plate"
    if (plateHeight > 0.15f) {
      // Land
      finalHeight = 0.2f + (plateHeight * 0.8f); // Base lift

      // Mountain Ranges at collision zones (edges of high randomness)
      if (breakup > 0.5f) {
        finalHeight += (breakup - 0.5f) * 0.5f;
      }
    } else {
      // Ocean
      finalHeight = plateHeight; // Deep trench at 0
    }

    // Detail
    finalHeight += detail.GetNoise((float)x, (float)y) * 0.05f;

    b.height[i] = clamp_val(finalHeight, 0.0f, 1.0f);
  }
}

// --- NEW FEATURES ---

void LoadHeightmapDataWithKeys(
    const char *path, WorldBuffers &buffers, uint32_t count,
    const std::vector<TerrainController::ColorKey> &keys) {
  int w, h, channels;
  unsigned char *data = stbi_load(path, &w, &h, &channels, 3); // Force RGB
  if (!data)
    return;

  int side = (int)std::sqrt(count);

  for (int i = 0; i < (int)count; ++i) {
    int x = i % side;
    int y = i / side;

    // Sample UV
    int imgX = (int)((float)x / (float)side * w);
    int imgY = (int)((float)y / (float)side * h);

    int idx = (imgY * w + imgX) * 3;
    unsigned char r = data[idx];
    unsigned char g = data[idx + 1];
    unsigned char b = data[idx + 2];

    // Find closest key
    float minDist = 1000000.0f;
    float bestH = 0.0f;

    for (const auto &k : keys) {
      float dr = (float)r - (float)k.r;
      float dg = (float)g - (float)k.g;
      float db = (float)b - (float)k.b;
      float dist = dr * dr + dg * dg + db * db;
      if (dist < minDist) {
        minDist = dist;
        bestH = k.targetHeight;
      }
    }

    buffers.height[i] = bestH;
  }

  stbi_image_free(data);
}

void TerrainController::LoadHeightmapFromImageWithKeys(
    WorldBuffers &b, const std::string &filepath,
    const std::vector<ColorKey> &keys) {
  LoadHeightmapDataWithKeys(filepath.c_str(), b, b.count, keys);
}

#include "../../include/AssetManager.hpp" // Needed for AutoPopulate

void TerrainController::AutoPopulate(WorldBuffers &b, const WorldSettings &s) {
  b.ClearAgents(); // Assuming this exists or we do it manually
  for (int i = 0; i < (int)b.count; ++i) {
    b.population[i] = 0;
    b.cultureID[i] = -1;
  }

  // Simple Biome Map
  // 0: Deep Ocean, 1: Ocean, 2: Beach, 3: Scorched, 4: Desert, 5: Savanna, 6:
  // TRF, 7: Grass, 8: Forest, 9: Temp RF, 10: Taiga, 11: Tundra, 12: Snow, 13:
  // Mtn

  // --- DYNAMIC AUTO-POPULATE ---
  // Iterate every cell
  for (int i = 0; i < (int)b.count; ++i) {
    int biomeID = b.biomeID[i];

    // 1. Get Biome Properties
    // AssetManager::biomeRegistry might not match IDs 1:1 if custom,
    // but we assume standard ID mapping for now or linear search.
    const BiomeDef *biome = nullptr;
    for (const auto &bd : AssetManager::biomeRegistry) {
      if (bd.id == biomeID) {
        biome = &bd;
        break;
      }
    }

    if (!biome)
      continue; // Biome not found in rules

    // 2. Identify Potential Habitats
    // Skip water for now unless we add aquatic agents
    if (biome->minHeight < s.seaLevel)
      continue;

    // 3. Find Suitable Agents
    // We look for FLORA first as the base layer
    std::vector<int> suitableFlora;
    std::vector<int> suitableFauna;

    float cellTemp = b.temperature[i];
    float cellMoist = b.moisture[i];

    for (const auto &agent : AssetManager::agentRegistry) {
      // Check Environmental Fit
      bool tempFit =
          (cellTemp >= agent.deadlyTempLow && cellTemp <= agent.deadlyTempHigh);
      // Moisture fit? Agent definition implies idealMoisture but maybe not
      // strict limits in struct? Let's use IdealTemp proximity as a weight.

      if (tempFit) {
        if (agent.type == AgentType::FLORA)
          suitableFlora.push_back(agent.id);
        if (agent.type == AgentType::FAUNA)
          suitableFauna.push_back(agent.id);
      }
    }

    // 4. Spawn Logic
    // Bias towards Flora first
    if (!suitableFlora.empty() &&
        (rand() % 100 < 60)) { // 60% chance for plant life
      int idx = rand() % suitableFlora.size();
      b.cultureID[i] = suitableFlora[idx];
      b.population[i] = 100 + rand() % 900;
    } else if (!suitableFauna.empty() &&
               (rand() % 100 <
                5)) { // 5% chance for animals (if no plants, or rare)
      int idx = rand() % suitableFauna.size();
      b.cultureID[i] = suitableFauna[idx];
      b.population[i] = 10 + rand() % 90;
    }
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
