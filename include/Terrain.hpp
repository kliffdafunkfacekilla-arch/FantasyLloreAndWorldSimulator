#pragma once
#include "WorldEngine.hpp"
#include <string>

class TerrainController {
public:
  // Generation
  static void GenerateHeightmap(WorldBuffers &b, const WorldSettings &s);
  static void GenerateClimate(WorldBuffers &b,
                              const WorldSettings &s); // Temp/Moisture maps
  static void SimulateHydrology(WorldBuffers &b,
                                const WorldSettings &s); // Rivers

  // NEW: The "Nortantis" Style Generator (Voronoi Plates)
  static void GenerateTectonicPlates(WorldBuffers &b, const WorldSettings &s);

  // Tools
  static void ApplyBrush(WorldBuffers &b, int width, int cx, int cy, float r,
                         float str, int mode);
  static void LoadHeightmapFromImage(WorldBuffers &b,
                                     const std::string &filepath);

  // Advanced Import
  struct ColorKey {
    unsigned char r, g, b;
    float targetHeight;
  };
  static void LoadHeightmapFromImageWithKeys(WorldBuffers &b,
                                             const std::string &filepath,
                                             const std::vector<ColorKey> &keys);

  // Life
  static void AutoPopulate(WorldBuffers &b, const WorldSettings &s);

  // Analysis
  static void RecalculateBiomes(WorldBuffers &b); // Fill the biome buffer

  // Legacy/Compatibility for S.A.G.A. Architect
  static void GenerateProceduralTerrain(WorldBuffers &b,
                                        const WorldSettings &s) {
    GenerateHeightmap(b, s);

    // --- RESOURCE MAP POPULATION ---
    srand(s.seed + 555); // different random stream

    for (uint32_t i = 0; i < b.count; ++i) {
        if (b.resourceType) b.resourceType[i] = 0; // Empty by default
        if (b.resourceAmount) b.resourceAmount[i] = 0.0f;

        float h = b.height[i];
        if (h <= s.seaLevel) {
            // Ocean resources (Fish/Pearls)
            if ((rand() % 1000) < 5) { // 0.5% chance
                if (b.resourceType) b.resourceType[i] = 1; // ID 1: Fish/Water Resource
                if (b.resourceAmount) b.resourceAmount[i] = 100.0f + (rand() % 500);
            }
        } else if (h > 0.8f) {
            // High Mountains (Iron/Gold/Gems)
            int chance = rand() % 1000;
            if (chance < 10) { // 1% chance
                if (b.resourceType) b.resourceType[i] = 3; // ID 3: Precious Metals
                if (b.resourceAmount) b.resourceAmount[i] = 50.0f + (rand() % 200);
            } else if (chance < 30) { // 2% chance
                if (b.resourceType) b.resourceType[i] = 2; // ID 2: Iron/Base Metals
                if (b.resourceAmount) b.resourceAmount[i] = 200.0f + (rand() % 1000);
            }
        } else {
            // Plains/Forests/Hills
            int chance = rand() % 1000;
            if (chance < 15) { // 1.5% chance
                if (b.resourceType) b.resourceType[i] = 4; // ID 4: Wood/Timber
                if (b.resourceAmount) b.resourceAmount[i] = 500.0f + (rand() % 2000);
            } else if (chance < 20) { // 0.5% chance
                if (b.resourceType) b.resourceType[i] = 5; // ID 5: Wild Crops/Game
                if (b.resourceAmount) b.resourceAmount[i] = 100.0f + (rand() % 300);
            }
        }
    }
  }
  static void ApplyThermalErosion(WorldBuffers &b, int iterations);
  static void EnforceOceanEdges(WorldBuffers &b, int side, float fadeDist);
  static void SmoothTerrain(WorldBuffers &b, int side);
  static void RoughenCoastlines(WorldBuffers &b, int side, float seaLevel);
};
