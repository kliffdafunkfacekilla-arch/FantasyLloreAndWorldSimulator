#include "../../include/FastNoiseLite.h"
#include "../../include/SimulationModules.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

void TerrainController::GenerateProceduralTerrain(
    WorldBuffers &buffers, const WorldSettings &settings) {

  std::cout << "[TERRAIN] Generating Hybrid Terrain v2...\n";

  // 1. SETUP GENERATORS
  // A. Base Shape (The Coastlines)
  FastNoiseLite baseNoise;
  baseNoise.SetSeed(settings.seed);
  baseNoise.SetFrequency(settings.continentFreq);
  baseNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);

  // B. Mountain Location (Where do mountains exist?)
  // We use a separate low-freq noise to draw "zones" for mountain ranges
  FastNoiseLite mountainMaskNoise;
  mountainMaskNoise.SetSeed(settings.seed + 101);
  mountainMaskNoise.SetFrequency(settings.continentFreq * 2.0f);
  mountainMaskNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);

  // C. Mountain Detail (The actual jagged spikes)
  FastNoiseLite mountainNoise;
  mountainNoise.SetSeed(settings.seed + 1);
  mountainNoise.SetFrequency(settings.featureFrequency);
  mountainNoise.SetFractalType(FastNoiseLite::FractalType_Ridged);
  mountainNoise.SetFractalOctaves(5);

  // D. Warper
  FastNoiseLite warper;
  warper.SetSeed(settings.seed + 2);
  warper.SetFrequency(settings.continentFreq);
  warper.SetDomainWarpType(FastNoiseLite::DomainWarpType_OpenSimplex2);
  warper.SetDomainWarpAmp(settings.warpStrength *
                          30.0f); // Boosted amp for visibility

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

  // 2. GENERATION LOOP
  for (uint32_t i = 0; i < settings.cellCount; ++i) {
    // Coordinate Setup
    float x = buffers.posX[i] * 100.0f;
    float y = buffers.posY[i] * 100.0f;

    // STEP A: Warp first (Distorts everything equally)
    if (settings.warpStrength > 0.0f) {
      warper.DomainWarp(x, y);
    }

    // STEP B: Get the distinct layers

    // 1. Continent Height (0.0 to 1.0)
    float baseH = (baseNoise.GetNoise(x, y) + 1.0f) * 0.5f;

    // 2. Mountain Mask (0.0 to 1.0) - Defines "Tectonic Zones"
    float mask = (mountainMaskNoise.GetNoise(x + 500, y + 500) + 1.0f) * 0.5f;

    // 3. Mountain Spikes (0.0 to 1.0) - The rough texture
    float peaks = (mountainNoise.GetNoise(x, y) + 1.0f) * 0.5f;
    peaks = std::pow(peaks, 2.0f); // Make them pointy

    // STEP C: The New Mixing Logic

    // Start with the flat ground
    float finalHeight = baseH;

    // "Mountain Influence" now controls the THRESHOLD of the mask.
    // Low Influence = Mountains only in very specific spots (ranges)
    // High Influence = Mountains everywhere
    // We remap the mask so that only the "hotspots" get mountains.
    float mountZone = std::max(0.0f, mask - 0.4f) * 2.0f; // Clip bottom 40%

    // If we are on land (baseH > seaLevel) OR if mountains are huge enough to
    // form islands
    if (baseH > settings.seaLevel - 0.05f) {
      // Add mountains ON TOP, unconstrained by base height
      // We multiply by mountZone so they fade in naturally at the edges of
      // ranges
      float mountainHeight = peaks * mountZone * settings.mountainInfluence;

      // Add to base, but don't just multiply. Stack them.
      finalHeight += mountainHeight;
    }

    // STEP D: Post-Process
    finalHeight *= settings.heightMultiplier;

    // Terracing (Optional stepped terrain effect)
    if (settings.terraceSteps > 0.0f) {
      float steps = settings.terraceSteps * 10.0f;
      finalHeight = std::round(finalHeight * steps) / steps;
    }

    // Soft Clamp (Prevent flat-topping)
    finalHeight =
        std::clamp(finalHeight, settings.heightMin, settings.heightMax);

    if (buffers.height)
      buffers.height[i] = finalHeight;

    // --- BIOME UPDATE (Simplified) ---
    if (buffers.temperature) {
      // Higher = Colder
      float altitudeCooling = finalHeight * 0.7f;
      float latTemp = 1.0f - (std::abs(buffers.posY[i] - 0.5f) * 2.0f);
      buffers.temperature[i] =
          std::clamp(latTemp - altitudeCooling, 0.0f, 1.0f);
    }

    if (buffers.moisture) {
      // Simple noise for moisture
      buffers.moisture[i] =
          (baseNoise.GetNoise(x + 900, y + 900) + 1.0f) * 0.5f;
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
