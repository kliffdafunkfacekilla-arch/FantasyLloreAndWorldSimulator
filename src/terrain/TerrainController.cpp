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
      buffers.posX[i] = (float)(i % side) / (float)side;
      buffers.posY[i] = (float)(i / side) / (float)side;
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

    // D. Multiplier
    finalHeight *= settings.heightMultiplier;

    // E. Island Mode (Force ocean at edges with ORGANIC noise)
    if (settings.islandMode) {
      // Use radial distance for a more natural circular island shape
      float dx = buffers.posX[i] - 0.5f;
      float dy = buffers.posY[i] - 0.5f;
      float d =
          std::sqrt(dx * dx + dy * dy) * 2.0f; // 0 at center, 1 at corners

      // Add noise to the radial distance to make it organic (not a perfect
      // circle)
      float distortion = (baseNoise.GetNoise(buffers.posX[i] * 200.0f,
                                             buffers.posY[i] * 200.0f) +
                          1.0f) *
                         0.5f;
      float distortedDist = d + (distortion - 0.5f) * settings.edgeFalloff;

      // Smooth falloff mask
      float falloffThreshold = 1.0f - settings.edgeFalloff;
      float falloffMask = 1.0f;
      if (distortedDist > falloffThreshold) {
        float f =
            (distortedDist - falloffThreshold) / (1.0f - falloffThreshold);
        falloffMask = std::max(0.0f, 1.0f - f * f * (3.0f - 2.0f * f));
      }

      // Final margin check for absolute safety
      float px = buffers.posX[i];
      float py = buffers.posY[i];
      float marginNorm = settings.islandMargin / 1000.0f;
      if (px < marginNorm || px > 1.0f - marginNorm || py < marginNorm ||
          py > 1.0f - marginNorm) {
        falloffMask = 0.0f;
      }

      finalHeight *= falloffMask;
    }

    // Manual clamp for C++ compatibility
    finalHeight =
        std::max(settings.heightMin, std::min(settings.heightMax, finalHeight));

    if (buffers.height)
      buffers.height[i] = finalHeight;

    // --- CLIMATE CONTROLS ---
    if (buffers.temperature) {
      // Base Latitudinal Temp (Equator is hot, Poles are cold)
      float latTemp = 1.0f - (std::abs(buffers.posY[i] - 0.5f) * 2.0f);

      // Altitude Cooling
      float altitudeCooling = finalHeight * 0.5f;

      // Combine + Add User Setting
      float temp = 0.3f + 0.7f * latTemp - (finalHeight * 0.5f);
      buffers.temperature[i] = std::max(0.0f, std::min(1.0f, temp));
    }

    if (buffers.moisture) {
      // Simple moisture based on proximity to sea/temp
      float moisture = (finalHeight < 0.2f) ? 0.8f : 0.4f;
      buffers.moisture[i] = moisture;
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
  if (iterations <= 0)
    return;

  std::cout << "[TERRAIN] Apply Thermal Erosion (" << iterations
            << " passes)...\n";

  int side = (int)std::sqrt(buffers.count);
  if (side == 0)
    return;

  // Use temp buffer to prevent feedback loops (explosion fix)
  std::vector<float> tempHeight(buffers.count);
  for (uint32_t i = 0; i < buffers.count; ++i) {
    tempHeight[i] = buffers.height[i];
  }

  float talusAngle = 0.004f; // Minimum slope before slippage
  float erosionSpeed = 0.1f; // Only move 10% per tick (stability)

  for (int iter = 0; iter < iterations; ++iter) {
    for (uint32_t i = 0; i < buffers.count; ++i) {
      float h = buffers.height[i];
      int x = i % side;
      int y = i / side;

      // Check 4 neighbors
      int neighbors[] = {
          (x > 0) ? (int)i - 1 : -1, (x < side - 1) ? (int)i + 1 : -1,
          (y > 0) ? (int)i - side : -1, (y < side - 1) ? (int)i + side : -1};

      for (int k = 0; k < 4; ++k) {
        int nIdx = neighbors[k];
        if (nIdx == -1)
          continue;

        float nH = buffers.height[nIdx];
        float diff = h - nH;

        // Only move dirt if slope exceeds threshold
        if (diff > talusAngle) {
          float transfer = (diff - talusAngle) * erosionSpeed;

          // CRITICAL: Cap transfer to prevent explosion
          if (transfer > diff * 0.5f)
            transfer = diff * 0.5f;

          tempHeight[i] -= transfer;
          tempHeight[nIdx] += transfer;
        }
      }
    }

    // Write back for next iteration
    for (uint32_t i = 0; i < buffers.count; ++i) {
      buffers.height[i] = tempHeight[i];
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

void TerrainController::RaiseTerrain(WorldBuffers &buffers, int centerIdx,
                                     float radius, float speed) {
  if (!buffers.height || centerIdx < 0 || centerIdx >= (int)buffers.count)
    return;
  int side = (int)std::sqrt(buffers.count);
  int cx = centerIdx % side;
  int cy = centerIdx / side;
  int r = (int)std::ceil(radius);

  for (int y = cy - r; y <= cy + r; ++y) {
    for (int x = cx - r; x <= cx + r; ++x) {
      if (x >= 0 && x < side && y >= 0 && y < side) {
        int idx = y * side + x;
        float dx = (float)(x - cx);
        float dy = (float)(y - cy);
        float dist = std::sqrt(dx * dx + dy * dy);
        if (dist <= radius) {
          float falloff = 1.0f - (dist / radius);
          buffers.height[idx] += speed * falloff;
          if (buffers.height[idx] > 1.0f)
            buffers.height[idx] = 1.0f;
        }
      }
    }
  }
}

void TerrainController::LowerTerrain(WorldBuffers &buffers, int centerIdx,
                                     float radius, float speed) {
  if (!buffers.height || centerIdx < 0 || centerIdx >= (int)buffers.count)
    return;
  int side = (int)std::sqrt(buffers.count);
  int cx = centerIdx % side;
  int cy = centerIdx / side;
  int r = (int)std::ceil(radius);

  for (int y = cy - r; y <= cy + r; ++y) {
    for (int x = cx - r; x <= cx + r; ++x) {
      if (x >= 0 && x < side && y >= 0 && y < side) {
        int idx = y * side + x;
        float dx = (float)(x - cx);
        float dy = (float)(y - cy);
        float dist = std::sqrt(dx * dx + dy * dy);
        if (dist <= radius) {
          float falloff = 1.0f - (dist / radius);
          buffers.height[idx] -= speed * falloff;
          if (buffers.height[idx] < 0.0f)
            buffers.height[idx] = 0.0f;
        }
      }
    }
  }
}

void TerrainController::SmoothTerrainRegion(WorldBuffers &buffers,
                                            int centerIdx, float radius,
                                            float speed) {
  if (!buffers.height || centerIdx < 0 || centerIdx >= (int)buffers.count)
    return;
  int side = (int)std::sqrt(buffers.count);
  int cx = centerIdx % side;
  int cy = centerIdx / side;
  int r = (int)std::ceil(radius);

  for (int y = cy - r; y <= cy + r; ++y) {
    for (int x = cx - r; x <= cx + r; ++x) {
      if (x >= 0 && x < side && y >= 0 && y < side) {
        // ... (Logic remains same, just simplified or kept) ...
        // Re-using the logic from the snippet
        int idx = y * side + x;
        float dx = (float)(x - cx);
        float dy = (float)(y - cy);
        float dist = std::sqrt(dx * dx + dy * dy);
        if (dist <= radius) {
          float falloff = 1.0f - (dist / radius);
          // Simple blurring logic for region
          // (omitted for brevity in replacement, but I must provide full
          // content if replacing) functionality was: average with neighbors and
          // blend. I'll rewrite the body to be safe.

          float avg = 0.0f;
          int count = 0;
          for (int ny = y - 1; ny <= y + 1; ny++) {
            for (int nx = x - 1; nx <= x + 1; nx++) {
              if (nx >= 0 && nx < side && ny >= 0 && ny < side) {
                avg += buffers.height[ny * side + nx];
                count++;
              }
            }
          }
          if (count > 0) {
            avg /= count;
            buffers.height[idx] =
                buffers.height[idx] * (1.0f - speed * falloff) +
                avg * (speed * falloff);
          }
        }
      }
    }
  }
}

void TerrainController::SmoothTerrain(WorldBuffers &buffers, int width) {
  if (!buffers.height)
    return;
  std::vector<float> temp(buffers.count);
  for (uint32_t i = 0; i < buffers.count; ++i) {
    float sum = 0;
    int count = 0;
    int x = i % width;
    int y = i / width;
    for (int dy = -1; dy <= 1; ++dy) {
      for (int dx = -1; dx <= 1; ++dx) {
        int nx = x + dx;
        int ny = y + dy;
        if (nx >= 0 && nx < width && ny >= 0 && ny < width) {
          sum += buffers.height[ny * width + nx];
          count++;
        }
      }
    }
    temp[i] = sum / count;
  }
  for (uint32_t i = 0; i < buffers.count; ++i)
    buffers.height[i] = temp[i];
}

// --- NEW TERRAIN TOOLS ---

void TerrainController::EnforceOceanEdges(WorldBuffers &b, int width,
                                          float fadeDist) {
  if (!b.height)
    return;
  // Simple distance from center logic or edge logic
  // "fadeDist" is norm distance
  // But user passes "1000" as width.

  for (int y = 0; y < width; ++y) {
    for (int x = 0; x < width; ++x) {
      // Distance from nearest edge
      float dx = (float)std::min(x, width - 1 - x);
      float dy = (float)std::min(y, width - 1 - y);
      float dist = std::min(dx, dy);

      if (dist < fadeDist) {
        float factor = dist / fadeDist;
        // Fade towards deep ocean (-1.0) or sea level?
        // Let's fade towards -0.5f
        int idx = y * width + x;
        // b.height[idx] = b.height[idx] * factor - 0.5f * (1.0f - factor);
        b.height[idx] = b.height[idx] * factor + (-1.0f) * (1.0f - factor);
      }
    }
  }
}

void TerrainController::RoughenCoastlines(WorldBuffers &b, int width,
                                          float seaLevel) {
  if (!b.height)
    return;
  for (size_t i = 0; i < b.count; ++i) {
    if (std::abs(b.height[i] - seaLevel) < 0.05f) { // Narrow band
      float noise = ((rand() % 100) / 100.0f - 0.5f) * 0.1f;
      b.height[i] += noise;
    }
  }
}

void TerrainController::ApplyBrush(WorldBuffers &b, int width, int cx, int cy,
                                   float radius, float strength, int mode) {
  if (!b.height)
    return;
  int r = (int)radius;
  int r2 = r * r;

  // Target height for Flatten mode (center of brush)
  float targetHeight = 0.0f;
  if (mode == 2) {
    int centerIdx = cy * width + cx;
    if (centerIdx >= 0 && centerIdx < (int)b.count)
      targetHeight = b.height[centerIdx];
  }

  for (int y = cy - r; y <= cy + r; ++y) {
    for (int x = cx - r; x <= cx + r; ++x) {
      // Bounds check
      if (x < 0 || x >= width || y < 0 || y >= width)
        continue;

      // Circle check
      int dx = x - cx;
      int dy = y - cy;
      if (dx * dx + dy * dy > r2)
        continue;

      // Falloff (Stronger in center)
      float dist = std::sqrt((float)(dx * dx + dy * dy));
      float falloff = 1.0f - (dist / radius);
      if (falloff < 0)
        falloff = 0;

      int i = y * width + x;
      float change = strength * falloff * 0.05f; // Scale down for control

      switch (mode) {
      case 0: // RAISE
        b.height[i] += change;
        break;
      case 1: // LOWER
        b.height[i] -= change;
        break;
      case 2: // FLATTEN
        b.height[i] = b.height[i] +
                      (targetHeight - b.height[i]) * 0.1f * strength * falloff;
        break;
      case 3: // NOISE/ROUGHEN
        b.height[i] += ((rand() % 100) / 1000.0f - 0.05f) * strength * falloff;
        break;
      }
      // Clamp
      if (b.height[i] > 1.0f)
        b.height[i] = 1.0f;
      if (b.height[i] < -1.0f)
        b.height[i] = -1.0f;
    }
  }
}
