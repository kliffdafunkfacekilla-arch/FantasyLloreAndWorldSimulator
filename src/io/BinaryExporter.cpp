#include "../../include/WorldEngine.hpp"
#include <fstream>
#include <iostream>

// The "Decoupled Bridge" (Binary Save)
// This function creates the static file that your Lore Wiki reads.
void SaveWorldSnapshot(const WorldBuffers& buffers, uint32_t count) {
    std::ofstream outFile("saves/world_state.dat", std::ios::binary);
    
    // Write Header (Cell Count and what modules are active)
    outFile.write((char*)&count, sizeof(uint32_t));

    // Bulk write the arrays (This is extremely fast)
    outFile.write((char*)buffers.height, count * sizeof(float));
    
    if (buffers.population) {
        outFile.write((char*)buffers.population, count * sizeof(uint32_t));
    }
    
    outFile.close();
    std::cout << "[SNAPSHOT] World State saved for Lore App." << std::endl;
}
