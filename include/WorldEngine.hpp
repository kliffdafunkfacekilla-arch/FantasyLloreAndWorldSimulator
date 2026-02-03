#pragma once

#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#include <cstdint> // Added for uint32_t

// 1. World Configuration (Linked to God Mode UI)
struct WorldSettings {
    // Core Generation
    uint32_t cellCount = 1000000;
    int seed = 1337;

    // Terrain & Import Controls
    char heightmapPath[256] = ""; // Fixed size for ImGui InputText compatibility
    float seaLevel = 0.2f;
    float heightMultiplier = 1.0f;
    float heightMin = 0.0f;
    float heightMax = 1.0f;
    float heightSeverity = 1.0f;    // Exponent for peak sharpness
    float featureFrequency = 0.01f; // Noise frequency
    float featureClustering = 2.0f; // Noise lacunarity

    // Climate & Wind (5-Zone System)
    bool manualWindOverride = false;
    float windZones[5] = {0.5f, -0.5f, 1.0f, -0.5f, 0.5f}; // N.Pole to S.Pole
    float globalTempModifier = 1.0f;
    float rainfallModifier = 1.0f;
    float globalWindStrength = 1.0f;

    // Erosion
    int erosionIterations = 10;

    // View & Scale (Zoom System)
    int zoomLevel = 0;          // 0=Global, 3=Local
    float pointSize = 1.0f;     // GPU point size
    float viewOffset[2] = {0.5f, 0.5f}; // Camera X,Y

    // Factions
    bool enableFactions = true;
};

// 2. The Million-Cell Memory (SoA Layout)
// using raw pointers for maximum CPU cache efficiency
struct WorldBuffers {
    // Core Geometry (Always Allocated)
    float* posX = nullptr;
    float* posY = nullptr;
    float* height = nullptr;

    // Simulation Layers (Allocated on Demand)
    float* temperature = nullptr;
    float* moisture = nullptr;
    float* windDX = nullptr; // Wind Vector X
    float* windDY = nullptr; // Wind Vector Y

    // Civilization & Life
    int* factionID = nullptr;
    uint32_t* population = nullptr;
    float* chaos = nullptr;
    float* infrastructure = nullptr; // Roads/Cities

    // Metadata
    uint32_t count = 0;

    // Lifecycle Management
    void Initialize(uint32_t c) {
        count = c;
        if (posX) Cleanup(); // Prevent double allocation

        posX = new float[count];
        posY = new float[count];
        height = new float[count];

        // Optional buffers can be initialized lazily,
        // but for now we allocate all for stability
        temperature = new float[count];
        moisture = new float[count];
        windDX = new float[count];
        windDY = new float[count];
        factionID = new int[count];
        population = new uint32_t[count];
        chaos = new float[count];
        infrastructure = new float[count];

        // Zero out memory
        std::fill_n(factionID, count, 0);
        std::fill_n(population, count, 0);
        std::fill_n(chaos, count, 0.0f);
        std::fill_n(infrastructure, count, 0.0f);
        std::fill_n(temperature, count, 0.0f);
        std::fill_n(moisture, count, 0.0f);
        std::fill_n(windDX, count, 0.0f);
        std::fill_n(windDY, count, 0.0f);
    }

    void Cleanup() {
        delete[] posX; delete[] posY; delete[] height;
        delete[] temperature; delete[] moisture;
        delete[] windDX; delete[] windDY;
        delete[] factionID; delete[] population;
        delete[] chaos; delete[] infrastructure;

        posX = nullptr; // Safety flag
    }
};

// 3. Time System
struct ChronosConfig {
    int dayCount = 0;
    int monthCount = 0;
    int yearCount = 0;
    int daysPerMonth = 30;
    float currentDayProgress = 0.0f;
    int globalTimeScale = 1; // 0 = Pause
};

// 4. Neighbor Graph (Added for Modules)
struct NeighborGraph {
    int* neighborData = nullptr;
    int* offsetTable = nullptr;
    uint8_t* countTable = nullptr;
};
