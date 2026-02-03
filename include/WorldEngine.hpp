#pragma once
#include <string>
#include <iostream>
#include <vector>
#include <cstdint>

// 1. User Settings (From Menu)
struct WorldSettings {
    uint32_t cellCount = 1000000;
    bool enableClimate = true;
    bool enableFactions = false;  // Optional
    bool enableChaos = false;     // Optional
    
    std::string heightmapPath = "";
    float globalTempModifier = 1.0f;
    float chaosInstabilityScale = 0.5f;
};

// 2. The Memory Buffers (SoA Style)
struct WorldBuffers {
    // Core Geometry
    float* posX = nullptr; 
    float* posY = nullptr;
    float* height = nullptr;

    // Optional Simulation Layers
    float* temperature = nullptr;
    float* moisture    = nullptr;
    uint32_t* population = nullptr;
    float* chaosEnergy   = nullptr;

    // Destructor to prevent memory leaks
    void clear() {
        delete[] posX; delete[] posY; delete[] height;
        delete[] temperature; delete[] moisture;
        delete[] population; delete[] chaosEnergy;
        std::cout << "[CLEANUP] World buffers deallocated." << std::endl;
    }
};
