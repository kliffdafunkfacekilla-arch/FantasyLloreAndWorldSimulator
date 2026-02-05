#include "../../include/FastNoiseLite.h"
#include "../../include/SimulationModules.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

void TerrainController::GenerateProceduralTerrain(
    WorldBuffers &buffers, const WorldSettings &settings) {

  std::cout << "[TERRAIN] Generating Fractal Terrain...\n";

  // 1. CONTINENTS (The "Blob" Fix - Using FBm)
  FastNoiseLite baseNoise;
  baseNoise.SetSeed(settings.seed);
  baseNoise.SetFrequency(settings.continentFreq);
  baseNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
  baseNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
  baseNoise.SetFractalOctaves(5);
  baseNoise.SetFractalLacunarity(2.0f);
  baseNoise.SetFractalGain(0.5f);

  // 2. MOUNTAIN ZONES
  FastNoiseLite mountainMaskNoise;
  mountainMaskNoise.SetSeed(settings.seed + 101);
  mountainMaskNoise.SetFrequency(settings.continentFreq * 2.0f);
  mountainMaskNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);

  // 3. MOUNTAIN PEAKS
  FastNoiseLite mountainNoise;
  mountainNoise.SetSeed(settings.seed + 1);
  mountainNoise.SetFrequency(settings.featureFrequency);
  mountainNoise.SetFractalType(FastNoiseLite::FractalType_Ridged);
  mountainNoise.SetFractalOctaves(5);

  // 4. WARP (Alien shapes)
  FastNoiseLite warper;
  warper.SetSeed(settings.seed + 2);
  warper.SetFrequency(settings.continentFreq);
  warper.SetDomainWarpType(FastNoiseLite::DomainWarpType_OpenSimplex2);
  warper.SetDomainWarpAmp(settings.warpStrength * 30.0f);

  // Initialize Grid positions
  int side = (int)std::sqrt(settings.cellCount);
  if (side == 0)
    side = 1000;

  for (uint32_t i = 0; i < settings.cellCount; ++i) {
    if (buffers.posX && buffers.posY) {
      buffers.posX[i] = (float)(i % side) / side;
      buffers.posY[i] = (float)(i / side) / side;
    }
  }

  for (uint32_t i = 0; i < settings.cellCount; ++i) {
    float x = buffers.posX[i] * 100.0f;
    float y = buffers.posY[i] * 100.0f;

    // Apply Warp
    if (settings.warpStrength > 0.0f) {
      warper.DomainWarp(x, y);
    }

    // A. Continents (Now Fractal!)
    float baseH = (baseNoise.GetNoise(x, y) + 1.0f) * 0.5f;

    // B. Mountains
    float mask = (mountainMaskNoise.GetNoise(x + 500, y + 500) + 1.0f) * 0.5f;
    float peaks = (mountainNoise.GetNoise(x, y) + 1.0f) * 0.5f;
    peaks = std::pow(peaks, 2.0f);

    // C. Combine
    float finalHeight = baseH;
    float mountZone = std::max(0.0f, mask - 0.4f) * 2.0f;

    if (baseH > settings.seaLevel - 0.05f) {
      float mountainHeight = peaks * mountZone * settings.mountainInfluence;
      finalHeight += mountainHeight;
    }

    // D. Terracing & Clamping
    if (settings.terraceSteps > 0.0f) {
      float steps = settings.terraceSteps * 10.0f;
      finalHeight = std::round(finalHeight * steps) / steps;
    }
    finalHeight *= settings.heightMultiplier;

    // E. Island Mode (Force ocean at edges)
    if (settings.islandMode) {
      // Distance from edge (0 at edge, 0.5 at center)
      float px = buffers.posX[i];
      float py = buffers.posY[i];
      float edgeDistX = std::min(px, 1.0f - px);
      float edgeDistY = std::min(py, 1.0f - py);
      float edgeDist = std::min(edgeDistX, edgeDistY);

      // Smooth falloff mask (0 at edge, 1 beyond falloff threshold)
      float falloffMask = std::min(1.0f, edgeDist / settings.edgeFalloff);
      falloffMask = falloffMask * falloffMask; // Smooth curve (quadratic)

      finalHeight *= falloffMask;
    }

    finalHeight =
        std::clamp(finalHeight, settings.heightMin, settings.heightMax);

    if (buffers.height)
      buffers.height[i] = finalHeight;

    // --- CLIMATE CONTROLS ---
    if (buffers.temperature) {
      // Base Latitudinal Temp (Equator is hot, Poles are cold)
      float latTemp = 1.0f - (std::abs(buffers.posY[i] - 0.5f) * 2.0f);

      // Altitude Cooling
      float altitudeCooling = finalHeight * 0.5f;

      // Combine + Add User Setting
      float t = latTemp - altitudeCooling + settings.globalTemperature;
      buffers.temperature[i] = std::clamp(t, 0.0f, 1.0f);
    }

    if (buffers.moisture) {
      // Noise + User Setting
      float m = (baseNoise.GetNoise(x + 900, y + 900) + 1.0f) * 0.5f;
      m += settings.globalMoisture;
      buffers.moisture[i] = std::clamp(m, 0.0f, 1.0f);
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

void TerrainController::InvertHeights(WorldBuffers &buffers) {
  if (!buffers.height)
    return;
  std::cout << "[TERRAIN] Inverting Heightmap...\n";
  for (uint32_t i = 0; i < buffers.count; ++i) {
    buffers.height[i] = 1.0f - buffers.height[i];
  }
}
