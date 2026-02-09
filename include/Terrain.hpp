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

  // Tools
  static void ApplyBrush(WorldBuffers &b, int width, int cx, int cy, float r,
                         float str, int mode);
  static void LoadHeightmapFromImage(WorldBuffers &b,
                                     const std::string &filepath);

  // Analysis
  static void RecalculateBiomes(WorldBuffers &b); // Fill the biome buffer

  // Legacy/Compatibility for S.A.G.A. Architect
  static void GenerateProceduralTerrain(WorldBuffers &b,
                                        const WorldSettings &s) {
    GenerateHeightmap(b, s);
  }
  static void ApplyThermalErosion(WorldBuffers &b, int iterations);
  static void EnforceOceanEdges(WorldBuffers &b, int side, float fadeDist);
  static void SmoothTerrain(WorldBuffers &b, int side);
  static void RoughenCoastlines(WorldBuffers &b, int side, float seaLevel);
};
