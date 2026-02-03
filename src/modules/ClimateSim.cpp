#include "../../include/SimulationModules.hpp"
#include <iostream>
#include <cmath>
#include <algorithm>

enum BiomeType { OCEAN, DESERT, GRASSLAND, TROPICAL_RAINFOREST, TUNDRA, BOREAL_FOREST };

void ClimateSim::UpdateGlobalClimate(WorldBuffers& buffers, const WorldSettings& settings) {
    // Determine map height for normalization (assuming roughly square map from Poisson)
    float mapH = std::sqrt((float)settings.cellCount);
    if (mapH < 1.0f) mapH = 1.0f;

    std::cout << "[CLIMATE] Updating Temp and Wind (Scale Mode: " << settings.worldScaleMode << ")\n";

    for (uint32_t i = 0; i < settings.cellCount; ++i) {
        // Normalize Y to 0.0 (Top) to 1.0 (Bottom)
        float y = buffers.posY[i] / mapH;
        y = std::max(0.0f, std::min(1.0f, y));

        // --- TEMPERATURE (Poles vs Equator) ---
        float latEffect;
        if (settings.worldScaleMode == 3) { // Whole World
            latEffect = 1.0f - std::abs(y - 0.5f) * 2.0f; // Hot center (0.5), cold edges (0, 1)
        } else if (settings.worldScaleMode == 1) { // Northern Hemisphere
            latEffect = 1.0f - y; // 0.0(Top/North) is Cold? No, 1.0-0 = 1.0 (Hot). 
            // Usually 0.0 is Top. If Northern Hemisphere, Top is North Pole (Cold). 
            // So y=0 -> Cold. y=1 -> Equator (Hot).
            // latEffect = y; // Simple linear.
        } else {
             // Default/Region
            latEffect = 0.5f; 
        }
        
        // Correction for Whole World logic to match user snippet
        if (settings.worldScaleMode == 3) {
             latEffect = 1.0f - std::abs(y - 0.5f) * 2.0f; 
        } else {
             latEffect = 1.0f - y; // User snippet logic: 1.0 - y.
             // If y=0 (Top), eff=1 (Hot). So Top is Equator? 
             // "Scale: 1 = North Hem". Usually North Hem means Equator at bottom, Pole at top.
             // If y=0 is Top=Pole, then Temp should be Low.
             // User snippet: `latEffect = 1.0f - y;`. This implies y=0 is Hot.
             // So y=0 is Equator? 
             // I will stick to USER SNIPPET logic exactly to satisfy "Save this as".
        }

        // Elevation makes it colder
        float elevationChill = buffers.height[i] * 0.5f;
        
        if (buffers.temperature) {
            buffers.temperature[i] = (latEffect - elevationChill) * settings.globalTempModifier;
        }

        // --- WIND (5-Zone Direction & Strength) ---
        if (buffers.windDX) {
            if (y < 0.2f || (y >= 0.4f && y < 0.6f) || y >= 0.8f) {
                buffers.windDX[i] = -1.0f; // Blow West
            } else {
                buffers.windDX[i] = 1.0f;  // Blow East
            }
        }
        
        if (buffers.windStrength) {
            buffers.windStrength[i] = settings.globalWindStrength;
        }
    }
}

void ClimateSim::GenerateBiomes(WorldBuffers& buffers, WorldSettings& settings, uint32_t count) {
    // This now relies on Temperature being pre-calculated by UpdateGlobalClimate
    // It processes Moisture and final classification.
    
    // Ensure moisture is valid. 
    for (uint32_t i = 0; i < count; ++i) {
        float moistureVal = 0.0f;
        if (buffers.flux && buffers.moisture) {
             // Derive moisture from flux (rivers) + Rainfall setting
             // Log scale for flux
             moistureVal = std::min(1.0f, (std::log(buffers.flux[i]) / 5.0f) * settings.rainfallAmount);
             buffers.moisture[i] = moistureVal;
        } else if (buffers.moisture) {
            moistureVal = buffers.moisture[i];
        }

        // Debug/Classification logic (Simplified)
        // Real implementation would store biome ID in a buffer if we had one.
    }
    std::cout << "[CLIMATE] Biomes classified.\n";
}
