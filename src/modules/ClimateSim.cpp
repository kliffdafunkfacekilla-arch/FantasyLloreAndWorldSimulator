#include "../../include/SimulationModules.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>


enum BiomeType {
  OCEAN,
  DESERT,
  GRASSLAND,
  TROPICAL_RAINFOREST,
  TUNDRA,
  BOREAL_FOREST
};

void ClimateSim::UpdateGlobalClimate(WorldBuffers &buffers,
                                     const WorldSettings &settings) {
  // Determine map height for normalization (assuming roughly square map from
  // Poisson)
  float mapH = std::sqrt((float)settings.cellCount);
  if (mapH < 1.0f)
    mapH = 1.0f;

  std::cout << "[CLIMATE] Updating Temp and Wind (Scale Mode: "
            << settings.worldScaleMode << ")\n";

  for (uint32_t i = 0; i < settings.cellCount; ++i) {
    // Normalize Y to 0.0 (Top) to 1.0 (Bottom)
    float y =
        buffers.posY[i]; // posY is already normalized 0..1 in TerrainController

    // --- TEMPERATURE (Poles vs Equator) ---
    float latEffect = 0.5f;

    switch (settings.worldScaleMode) {
    case 0: // Region (Uniform Base)
      latEffect = 0.5f;
      break;
    case 1: // Northern Hemisphere (Top=Cold, Bottom=Hot)
      // y=0 (Top) -> Cold (0.0)
      // y=1 (Bottom) -> Hot (1.0)
      latEffect = y;
      break;
    case 2: // Southern Hemisphere (Top=Hot, Bottom=Cold)
      // y=0 (Top) -> Hot (1.0)
      // y=1 (Bottom) -> Cold (0.0)
      latEffect = 1.0f - y;
      break;
    case 3: // Whole World (Top=Cold, Mid=Hot, Bot=Cold)
      // y=0 -> 0.0
      // y=0.5 -> 1.0
      // y=1 -> 0.0
      // Distance from center (0.5). Max dist is 0.5.
      // 1.0 - (abs(y - 0.5) * 2)
      latEffect = 1.0f - std::abs(y - 0.5f) * 2.0f;
      break;
    default:
      latEffect = 0.5f;
      break;
    }

    // Elevation makes it colder
    float elevationChill = buffers.height[i] * 0.5f;

    if (buffers.temperature) {
      // Base Temp (0..1) modified by global modifier
      // Result can be negative (freezing)
      buffers.temperature[i] =
          (latEffect - elevationChill) * settings.globalTempModifier;
    }

    // --- WIND (Direction based on Latitude) ---
    // Simplified Cell Zones
    // 0..1 range.
    if (buffers.windDX) {
      float windDir = 1.0f; // East by default

      // Logic for Whole World Trade Winds approximation
      // 0.0-0.2: Polar Easterlies (East -> West) (Dir: -1)
      // 0.2-0.4: Westerlies (West -> East) (Dir: 1)
      // 0.4-0.6: Trade Winds (East -> West) (Dir: -1) [Equator Zone]
      // ... mirrored

      // Simplified for the chosen scale mode?
      // Let's stick to a generic latitude-based band lookup for now, roughly
      // applied.

      // Map the specific y (0..1) to efficient Global Latitude (0..180 or
      // similar) For now, let's keep the simple logic from before but refined.

      bool isEasterly = false;

      if (settings.worldScaleMode == 3) { // Whole World
        // Bands at 0-0.16, 0.16-0.33, 0.33-0.5...
        // 3 bands per hemisphere = 6 bands total.
        // Band 0 (0.0 - 0.16): East (-1)
        // Band 1 (0.16 - 0.33): West (1)
        // Band 2 (0.33 - 0.50): East (-1)
        // Band 3 (0.50 - 0.66): East (-1)
        // Band 4 (0.66 - 0.83): West (1)
        // Band 5 (0.83 - 1.00): East (-1)

        int band = (int)(y * 6.0f);
        if (band == 0 || band == 2 || band == 3 || band == 5)
          isEasterly = true;

      } else {
        // Hemisphere mode: 3 bands.
        int band = (int)(y * 3.0f);
        if (band == 0 || band == 2)
          isEasterly = true;
      }

      buffers.windDX[i] = isEasterly ? -1.0f : 1.0f;
    }

    if (buffers.windStrength) {
      buffers.windStrength[i] = settings.globalWindStrength;
    }
  }
}

void ClimateSim::GenerateBiomes(WorldBuffers &buffers, WorldSettings &settings,
                                uint32_t count) {
  // Ensure moisture is valid.
  for (uint32_t i = 0; i < count; ++i) {
    float moistureVal = 0.0f;
    if (buffers.flux && buffers.moisture) {
      // Derive moisture from flux (rivers) + Rainfall setting
      // Log scale for flux
      moistureVal = std::min(1.0f, (std::log(buffers.flux[i]) / 5.0f) *
                                       settings.rainfallAmount);
      buffers.moisture[i] = moistureVal;
    } else if (buffers.moisture) {
      moistureVal = buffers.moisture[i];

      // Apply rainfall modifier specifically here if flux missing
      if (!buffers.flux)
        moistureVal *= settings.rainfallAmount;
    }
  }
  std::cout << "[CLIMATE] Biomes classified.\n";
}
