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

  std::cout << "[TERRAIN] Generating Hybrid Terrain (Warp + Continents + "
               "Mountains)...\n";

  // 1. SETUP THE THREE GENERATORS

  // A. Base Shape (The Continents)
  FastNoiseLite baseNoise;
  baseNoise.SetSeed(settings.seed);
  baseNoise.SetFrequency(settings.continentFreq); // Low frequency
  baseNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);

  // B. Mountain Detail (The Peaks)
  FastNoiseLite mountainNoise;
  mountainNoise.SetSeed(settings.seed + 1); // Offset seed
  mountainNoise.SetFrequency(settings.featureFrequency);
  mountainNoise.SetFractalType(FastNoiseLite::FractalType_RidgedMulti);
  mountainNoise.SetFractalOctaves(5);

  // C. Domain Warper (The Fluid Distortion)
  FastNoiseLite warper;
  warper.SetSeed(settings.seed + 2);
  warper.SetFrequency(settings.continentFreq);
  warper.SetDomainWarpType(FastNoiseLite::DomainWarpType_OpenSimplex2);
  // We handle amplitude manually via warpStrength * coordinate scale

  int side = (int)std::sqrt(settings.cellCount);
  if (side == 0)
    side = 1000;

  // Initialize Grid (if needed, mostly redundant if already init)
  for (uint32_t i = 0; i < settings.cellCount; ++i) {
    if (buffers.posX && buffers.posY) {
      buffers.posX[i] = (float)(i % side) / side;
      buffers.posY[i] = (float)(i / side) / side;
    }
  }

  for (uint32_t i = 0; i < settings.cellCount; ++i) {
    // Get Base Coordinates
    float x = buffers.posX[i] * 100.0f;
    float y = buffers.posY[i] * 100.0f;

    // STEP A: Apply Domain Warping
    // This twists X and Y in place before we generate height.
    // We scale the warp effect by the UI slider.
    // Note: FastNoiseLite DomainWarp takes reference refs float& x, float& y
    if (settings.warpStrength > 0.0f) {
      // Basic domain warp doesn't expose amplitude directly in simple calls
      // easily without configuring the fractal But SetDomainWarpAmp is not
      // always reliable in all FNL versions. Instead we can just configure the
      // warper to be strong or multiply the input/output manually? Actually FNL
      // DomainWarp works by adding noise to coords. We'll configure fractal
      // settings or just rely on the frequency. For user control
      // 'warpStrength', let's set FractalGain or Type? Simpler: Just rely on
      // the Warper object as configured. Wait, user asked for: float
      // currentWarp = s.warpStrength * 20.0f; FNL doesn't take 'strength' in
      // the DomainWarp call. We will trust the user's snippet intent but adapt
      // to FNL API. FNL usually uses SetDomainWarpAmp.
      warper.SetDomainWarpAmp(settings.warpStrength * 20.0f);
      warper.DomainWarp(x, y);
    }

    // STEP B: Generate Base Ground (Continents)
    float shape = baseNoise.GetNoise(x, y);
    shape = (shape + 1.0f) * 0.5f; // Normalize 0-1

    // STEP C: Generate Mountains (Ridged)
    float detail = mountainNoise.GetNoise(x, y);
    detail = (detail + 1.0f) * 0.5f; // Normalize 0-1
    detail = std::pow(detail, 2.0f); // Sharpen the peaks further

    // STEP D: Combine (Masking)
    // Low shape = Ocean, High shape = Land
    float finalHeight = shape;

    // Only add mountains if we are essentially on land/high ground
    if (shape > 0.3f) {
      // The higher the landmass, the more mountain detail we allow.
      finalHeight += detail * shape * settings.mountainInfluence;
    }

    // STEP E: Final Cleanup
    finalHeight *= settings.heightMultiplier;
    finalHeight =
        std::clamp(finalHeight, settings.heightMin, settings.heightMax);

    // Soft clamp to [0,1] for safety
    finalHeight = std::max(0.0f, std::min(finalHeight, 1.0f));

    if (buffers.height)
      buffers.height[i] = finalHeight;

    // --- RETAIN BIOME LOGIC ---
    // Basic Temperature
    if (buffers.temperature) {
      float latitude = std::abs(buffers.posY[i] - 0.5f) * 2.0f;
      float temp = 1.0f - latitude;
      temp -= finalHeight * 0.5f;
      buffers.temperature[i] = std::clamp(temp, 0.0f, 1.0f);
    }

    // Basic Moisture
    if (buffers.moisture) {
      // Use the warped coordinates for moisture too!
      float m = baseNoise.GetNoise(x + 500.0f,
                                   y + 500.0f); // Re-use base noise with offset
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
