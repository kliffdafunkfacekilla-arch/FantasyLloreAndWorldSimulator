#include "../../include/SimulationModules.hpp"
#include <cmath>
#include <algorithm>
#include <iostream>

namespace ClimateSim {

    void Update(WorldBuffers& b, const WorldSettings& s) {
        // 1. CLEAR OLD DATA (Optional, but good for "God Mode" responsiveness)
        // If we don't clear, wind changes might lag behind visual updates

        for (uint32_t i = 0; i < s.cellCount; ++i) {
            float y = b.posY[i]; // 0.0 (North) to 1.0 (South)

            // --- A. TEMPERATURE CALCULATION ---
            // Formula: Base - (Latitude_Distance) - (Altitude * LapseRate)
            float latDist = (s.zoomLevel == 3) ? 0.0f : std::abs(y - 0.5f) * 2.0f; // 0.0 at Equator
            float altitudeChill = b.height[i] * 0.5f;

            b.temperature[i] = (1.0f - latDist - altitudeChill) * s.globalTempModifier;

            // --- B. 5-ZONE WIND SYSTEM ---
            if (s.manualWindOverride) {
                // Map 0.0-1.0 Y-coord to 0-4 Array Index
                int zone = std::clamp((int)(y * 5.0f), 0, 4);
                b.windDX[i] = s.windZones[zone];
            } else {
                // Default Trade Winds logic (Simulates Hadley Cells)
                // Sine wave creates alternating East/West bands
                b.windDX[i] = (sin(y * 9.42f) > 0) ? 1.0f : -1.0f;
            }

            // Apply Global Wind Strength Multiplier
            b.windDX[i] *= s.globalWindStrength;

            // --- C. MOISTURE & RAINFALL ---
            // Simple approximation: If over water (height < seaLevel), grab moisture.
            // If wind brings it to land, drop it.
            if (b.height[i] < s.seaLevel) {
                b.moisture[i] = 1.0f;
            } else {
                // Decay moisture inland (Basic Rain Shadow)
                b.moisture[i] *= 0.98f;
                if (b.windDX[i] != 0) {
                    // Check previous cell (simplified) - Real version uses NeighborGraph
                    // For visualization speed, we just add global rainfall mod
                    b.moisture[i] += s.rainfallModifier * 0.01f;
                }
            }

            // Clamp values
            b.temperature[i] = std::clamp(b.temperature[i], 0.0f, 1.0f);
            b.moisture[i] = std::clamp(b.moisture[i], 0.0f, 1.0f);
        }
    }
}
