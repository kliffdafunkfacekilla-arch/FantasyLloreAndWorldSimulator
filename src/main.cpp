#include <iostream>

// In this simple structure, we are including cpp files directly 
// because we haven't created separate headers for them yet.
#include "../include/WorldEngine.hpp"
#include "core/MemoryManager.cpp"
#include "io/BinaryExporter.cpp"
#include "core/SimulationLoop.cpp" 

int main() {
    std::cout << "OmnisWorldEngine Starting..." << std::endl;

    // 1. Setup Configuration
    WorldSettings settings;
    settings.cellCount = 1000000;
    settings.enableClimate = true;
    settings.enableChaos = true;

    // 2. Initialize Memory
    WorldBuffers buffers;
    MemoryManager memManager;
    memManager.InitializeWorld(settings, buffers);

    // 3. Run Simulation (Placeholder)
    SimulationLoop sim;
    sim.run();

    // 4. Export State
    SaveWorldSnapshot(buffers, settings.cellCount);

    // 5. Cleanup
    buffers.clear();
    
    return 0;
}
