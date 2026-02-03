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
      latEffect = y;
      break;
    case 2: // Southern Hemisphere (Top=Hot, Bottom=Cold)
      latEffect = 1.0f - y;
      break;
    case 3: // Whole World (Top=Cold, Mid=Hot, Bot=Cold)
      latEffect = 1.0f - std::abs(y - 0.5f) * 2.0f;
      break;
    default:
      latEffect = 0.5f;
      break;
    }

    // Elevation makes it colder
    float elevationChill = buffers.height[i] * 0.5f;

    if (buffers.temperature) {
      buffers.temperature[i] =
          (latEffect - elevationChill) * settings.globalTempModifier;
    }

    // --- WIND (Direction based on Latitude) ---
    if (buffers.windDX) {
      float windDir = 1.0f; // East (+1) or West (-1)

      if (settings.manualWindOverride) {
        // Determine which zone the cell is in (0 to 4)
        int zoneIndex = (int)(y * 5.0f);
        zoneIndex = std::clamp(zoneIndex, 0, 4);
        buffers.windDX[i] = settings.windZoneWeights[zoneIndex];
      } else {
        bool isEasterly = false;
        if (settings.worldScaleMode == 3) { // Whole World
          int band = (int)(y * 6.0f);
          if (band == 0 || band == 2 || band == 3 || band == 5)
            isEasterly = true;
        } else {
          int band = (int)(y * 3.0f);
          if (band == 0 || band == 2)
            isEasterly = true;
        }
        buffers.windDX[i] = isEasterly ? -1.0f : 1.0f;
      }
    }

    if (buffers.windStrength) {
      buffers.windStrength[i] = settings.globalWindStrength;
    }
  }
}

void ClimateSim::GenerateBiomes(WorldBuffers &buffers, WorldSettings &settings,
                                uint32_t count) {
  for (uint32_t i = 0; i < count; ++i) {
    float moistureVal = 0.0f;
    if (buffers.flux && buffers.moisture) {
      moistureVal = std::min(1.0f, (std::log(buffers.flux[i]) / 5.0f) *
                                       settings.rainfallAmount);
      buffers.moisture[i] = moistureVal;
    } else if (buffers.moisture) {
      moistureVal = buffers.moisture[i];
      if (!buffers.flux)
        moistureVal *= settings.rainfallAmount;
    }
  }
  std::cout << "[CLIMATE] Biomes classified.\n";
}
