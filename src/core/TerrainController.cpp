#include "../../include/SimulationModules.hpp"
#include "../core/FastNoiseLite.h" // Relative path to where we downloaded it
#include <cmath>
#include <algorithm>
#include <iostream>

void TerrainController::GenerateTerrain(WorldBuffers& buffers, const WorldSettings& settings) {
     if (!settings.heightmapPath.empty()) {
        std::cout << "[TERRAIN] Loading heightmap from " << settings.heightmapPath << "\n";
        LoadHeightmapData(settings.heightmapPath.c_str(), buffers, settings.cellCount);
        return; 
    }

    std::cout << "[TERRAIN] Generating procedural terrain with Seed " << settings.seed << "\n";

    // We use a noise library like FastNoiseLite for the high spots
    FastNoiseLite noise;
    noise.SetSeed(settings.seed);
    noise.SetFrequency(settings.featureFrequency);
    noise.SetFractalLacunarity(settings.featureClustering);
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);

    for (uint32_t i = 0; i < settings.cellCount; ++i) {
        // 1. Get Base Noise
        // Use posX/posY. Assuming they are in a reasonable range (0..Width).
        float h = noise.GetNoise(buffers.posX[i], buffers.posY[i]);
        
        // Normalize noise from -1..1 to 0..1
        h = (h + 1.0f) * 0.5f;

        // 2. Adjust Severity (The "Gentle vs Severe" slider)
        // Ensure h is positive basic sanity
        h = std::pow(h, settings.heightSeverity);
        
        // 3. Apply Multiplier and Min/Max Clamping
        h *= settings.heightMultiplier;
        h = std::clamp(h, settings.heightMin, settings.heightMax);
        
        buffers.height[i] = h;
    }
}

void TerrainController::RunThermalErosion(WorldBuffers& buffers, const NeighborGraph& graph, int iterations, uint32_t count) {
    float talusThreshold = 0.01f; 
    float moveAmount = 0.005f;

    for (int iter = 0; iter < iterations; ++iter) {
        for (uint32_t i = 0; i < count; ++i) {
            int offset = graph.offsetTable[i];
            uint8_t n_count = graph.countTable[i];

            for (int n = 0; n < n_count; ++n) {
                int neighborID = graph.neighborData[offset + n];
                float diff = buffers.height[i] - buffers.height[neighborID];

                if (diff > talusThreshold) {
                    buffers.height[i] -= moveAmount;
                    buffers.height[neighborID] += moveAmount;
                }
            }
        }
    }
    std::cout << "[TERRAIN] Erosion pass complete (" << iterations << " iterations).\n";
}
