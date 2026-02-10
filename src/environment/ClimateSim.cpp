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
    return BiomeType::TEMPERATE_DECIDUOUS_FOREST;
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
  int side = (int)std::sqrt(b.count);
  if (side == 0)
    return;

  // Noise generators for variation
  FastNoiseLite tempNoise;
  tempNoise.SetFrequency(0.003f);
  FastNoiseLite rainNoise;
  rainNoise.SetFrequency(0.005f);

  for (int i = 0; i < (int)b.count; ++i) {
    int x = i % side;
    int y = i / side;
    float h = b.height[i];

    // --- 1. TEMPERATURE (3-ZONE LERP) ---
    float lat = (float)y / side; // 0.0 (N) to 1.0 (S)
    float baseTemp = 0.0f;

    if (lat < 0.5f) {
      float alpha = lat / 0.5f;
      baseTemp = s.tempZonePolar * (1.0f - alpha) + s.tempZoneTemperate * alpha;
    } else {
      float alpha = (lat - 0.5f) / 0.5f;
      baseTemp =
          s.tempZoneTemperate * (1.0f - alpha) + s.tempZoneTropical * alpha;
    }

    // Modifier: Altitude (Higher is colder)
    float altMod = std::max(0.0f, h - s.seaLevel) * 0.8f;
    // Modifier: Noise
    float nT = tempNoise.GetNoise((float)x, (float)y) * 0.1f;

    b.temperature[i] = std::clamp(baseTemp - altMod + nT, 0.0f, 1.0f);

    // --- 2. WIND (5-ZONE MAPPING) ---
    int windZoneIdx = std::clamp((int)(lat * 5.0f), 0, 4);
    float localWindAngle = s.windZonesDir[windZoneIdx];
    float localWindStrength = s.windZonesStr[windZoneIdx];

    float windX = std::cos(localWindAngle) * localWindStrength;
    float windY = std::sin(localWindAngle) * localWindStrength;

    b.windDX[i] = windX;
    b.windDY[i] = windY;

    // --- 3. MOISTURE (RAIN SHADOW LOGIC) ---
    float moisture = 0.0f;

    // Sample upwind
    int uwX = x - (int)(windX * 15.0f);
    int uwY = y - (int)(windY * 15.0f);

    bool upwindIsOcean = true;
    float blockage = 0.0f;

    if (uwX >= 0 && uwX < side && uwY >= 0 && uwY < side) {
      int uwIdx = uwY * side + uwX;
      if (b.height[uwIdx] > s.seaLevel)
        upwindIsOcean = false;

      // Check for mountain obstruction
      int midX = (x + uwX) / 2;
      int midY = (y + uwY) / 2;
      int midIdx = midY * side + midX;
      if (b.height[midIdx] > s.seaLevel + 0.3f) // High peak
        blockage = 1.0f;
    }

    if (h <= s.seaLevel) {
      moisture = 1.0f;
    } else {
      if (upwindIsOcean && blockage < 0.5f)
        moisture += 0.6f * localWindStrength;
      else if (blockage > 0.5f)
        moisture -= 0.4f;
      else
        moisture -= 0.1f;

      moisture += rainNoise.GetNoise((float)x, (float)y) * 0.2f;
    }

    b.moisture[i] = std::clamp(moisture * s.raininess, 0.0f, 1.0f);

    // --- 4. BIOME CLASSIFICATION ---
    if (h <= s.seaLevel) {
      b.biomeID[i] = BiomeType::OCEAN;
    } else {
      b.biomeID[i] = GetBiome(b.temperature[i], b.moisture[i]);
    }
  }
}
} // namespace ClimateSim
