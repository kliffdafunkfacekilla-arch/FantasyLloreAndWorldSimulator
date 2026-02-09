#include "../../include/Environment.hpp"
#include "../../include/FastNoiseLite.h"
#include <algorithm>
#include <cmath>
#include <iostream>


namespace ClimateSim {

// Helper: Whittaker Diagram Lookup
int GetBiome(float temp, float moisture) {
  if (temp < 0.1f)
    return BiomeType::SNOW;
  if (temp < 0.2f)
    return BiomeType::TUNDRA;
  if (temp < 0.4f) {
    if (moisture < 0.3f)
      return BiomeType::BARE;
    if (moisture < 0.6f)
      return BiomeType::SHRUBLAND;
    return BiomeType::TEMPERATE_DECIDUOUS_FOREST; // ताएगा approximation
  }
  if (temp < 0.7f) {
    if (moisture < 0.2f)
      return BiomeType::TEMPERATE_DESERT;
    if (moisture < 0.5f)
      return BiomeType::GRASSLAND;
    if (moisture < 0.8f)
      return BiomeType::TEMPERATE_DECIDUOUS_FOREST;
    return BiomeType::TEMPERATE_RAIN_FOREST;
  }
  // Hot
  if (moisture < 0.2f)
    return BiomeType::SCORCHED;
  if (moisture < 0.4f)
    return BiomeType::SUBTROPICAL_DESERT;
  if (moisture < 0.7f)
    return BiomeType::TROPICAL_SEASONAL_FOREST;
  return BiomeType::TROPICAL_RAIN_FOREST;
}

void Update(WorldBuffers &b, const WorldSettings &s) {
  int side = std::sqrt(b.count);
  if (side == 0)
    return;

  float windX = std::cos(s.windAngle);
  float windY = std::sin(s.windAngle);

  // Noise generators for variation
  FastNoiseLite tempNoise;
  tempNoise.SetFrequency(0.003f);
  FastNoiseLite rainNoise;
  rainNoise.SetFrequency(0.005f);

  for (int i = 0; i < (int)b.count; ++i) {
    int x = i % side;
    int y = i / side;
    float h = b.height[i];

    // --- 1. TEMPERATURE ---
    // Base: Latitude (Pole to Equator)
    float latitude = (float)y / side;
    float baseTemp = 1.0f - std::abs(latitude - 0.5f) * 2.0f;

    // Modifier: Altitude (Higher is colder)
    float altMod = std::max(0.0f, h - s.seaLevel) * 0.8f;

    // Modifier: Noise & Global Setting
    float nT = tempNoise.GetNoise((float)x, (float)y) * 0.1f;

    b.temperature[i] =
        std::clamp(baseTemp - altMod + nT + s.globalTemp, 0.0f, 1.0f);

    // --- 2. MOISTURE & WIND ---
    // "Look Upwind" to see if we are blocked by mountains
    float moisture = 0.0f;

    // Sample a point "upwind"
    int uwX = x - (int)(windX * 15.0f); // 15 tiles away
    int uwY = y - (int)(windY * 15.0f);

    // Is the upwind spot Ocean or Land?
    bool upwindIsOcean = true;
    float blockage = 0.0f;

    if (uwX >= 0 && uwX < side && uwY >= 0 && uwY < side) {
      int uwIdx = uwY * side + uwX;
      if (b.height[uwIdx] > s.seaLevel)
        upwindIsOcean = false;

      // Check for mountain obstruction (Rain Shadow) between here and there
      int midX = (x + uwX) / 2;
      int midY = (y + uwY) / 2;
      int midIdx = midY * side + midX;
      if (b.height[midIdx] > 0.7f)
        blockage = 1.0f; // Blocked by peak
    }

    // Calculate Base Rainfall
    if (h <= s.seaLevel) {
      moisture = 1.0f; // Ocean is wet
    } else {
      // If wind comes from ocean and not blocked -> Wet
      if (upwindIsOcean && blockage < 0.5f)
        moisture += 0.6f;
      // If blocked -> Dry (Shadow)
      else
        moisture -= 0.3f;

      // Add Noise variation
      moisture += rainNoise.GetNoise((float)x, (float)y) * 0.2f;
    }

    // Apply Global Rain Settings
    b.moisture[i] = std::clamp(moisture * s.raininess, 0.0f, 1.0f);

    // --- 3. BIOME CLASSIFICATION ---
    if (h <= s.seaLevel) {
      b.biomeID[i] = BiomeType::OCEAN;
    } else {
      b.biomeID[i] = GetBiome(b.temperature[i], b.moisture[i]);
    }
  }
}
} // namespace ClimateSim
