#include <fstream>
#include <iostream>
#include "../../include/WorldEngine.hpp"

void SaveWorldSnapshot(const WorldBuffers& buffers, uint32_t count, const char* filename) {
    std::ofstream outFile(filename, std::ios::binary);

    if (!outFile.is_open()) {
        std::cerr << "[ERROR] Could not create snapshot file." << std::endl;
        return;
    }

    // 1. Write Header (Cell Count)
    outFile.write(reinterpret_cast<const char*>(&count), sizeof(uint32_t));

    // 2. Dump Memory Blocks
    // We only save what is allocated. For a full app, you'd add flags.

    // Core Geometry
    if (buffers.height)
        outFile.write(reinterpret_cast<char*>(buffers.height), count * sizeof(float));

    // Simulation Data
    if (buffers.population)
        outFile.write(reinterpret_cast<char*>(buffers.population), count * sizeof(uint32_t));

    if (buffers.factionID)
        outFile.write(reinterpret_cast<char*>(buffers.factionID), count * sizeof(int));

    outFile.close();
    std::cout << "[SNAPSHOT] Saved world state to " << filename << std::endl;
}
