#pragma once
#include "WorldEngine.hpp"

// Terrain & Structure (src/terrain/TerrainController.cpp)
class TerrainController {
public:
  void LoadFromImage(const char *path, WorldBuffers &buffers);
  void ApplyThermalErosion(WorldBuffers &buffers, int iterations);
  void InvertHeights(WorldBuffers &buffers);
  // Helpers
  void GenerateProceduralTerrain(WorldBuffers &buffers,
                                 const WorldSettings &settings);

  // Modifiers
  void EnforceOceanEdges(WorldBuffers &b, int width, float fadeDist);
  void RoughenCoastlines(WorldBuffers &b, int width, float seaLevel);
  void SmoothTerrain(WorldBuffers &b, int width);

  // Interaction (Brush)
  // Mode: 0=Raise, 1=Lower, 2=Flatten, 3=Noise
  void ApplyBrush(WorldBuffers &b, int width, int cx, int cy, float radius,
                  float strength, int mode);

  // Legacy Paint Tools (Keep for compatibility or refactor to use ApplyBrush)
  void RaiseTerrain(WorldBuffers &buffers, int centerIdx, float radius,
                    float speed);
  void LowerTerrain(WorldBuffers &buffers, int centerIdx, float radius,
                    float speed);
  void SmoothTerrainExperimental(
      WorldBuffers &buffers, int centerIdx, float radius,
      float speed); // Renamed to avoid conflict if signature matches, or just
                    // remove if unused.
  // Actually, previous SmoothTerrain had signature (buffers, int idx, float r,
  // float s). The new one is (buffers, int width). I'll keep the old one as
  // `SmoothTerrainRegion` to avoid conflict.
  void SmoothTerrainRegion(WorldBuffers &buffers, int centerIdx, float radius,
                           float speed);
};

// IO Helpers
void LoadHeightmapData(const char *path, WorldBuffers &buffers, uint32_t count);
