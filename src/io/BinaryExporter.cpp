#include "../../include/BinaryExporter.hpp"
#include "../../include/WorldEngine.hpp"
#include <fstream>
#include <iostream>
#include <vector>

namespace BinaryExporter {

void SaveWorld(const WorldBuffers &buffers, const std::string &filename) {
  std::ofstream outFile(filename, std::ios::binary);

  if (!outFile.is_open()) {
    std::cerr << "[ERROR] Could not create map file: " << filename << std::endl;
    return;
  }

  // 1. Write Header
  uint32_t count = buffers.count;
  outFile.write(reinterpret_cast<const char *>(&count), sizeof(uint32_t));

  // 2. Dump Layers
  if (buffers.height)
    outFile.write(reinterpret_cast<const char *>(buffers.height),
                  count * sizeof(float));
  if (buffers.temperature)
    outFile.write(reinterpret_cast<const char *>(buffers.temperature),
                  count * sizeof(float));
  if (buffers.moisture)
    outFile.write(reinterpret_cast<const char *>(buffers.moisture),
                  count * sizeof(float));
  if (buffers.cultureID)
    outFile.write(reinterpret_cast<const char *>(buffers.cultureID),
                  count * sizeof(int));
  if (buffers.population)
    outFile.write(reinterpret_cast<const char *>(buffers.population),
                  count * sizeof(uint32_t));

  // NEW: Critical for Replay
  if (buffers.agentID)
    outFile.write(reinterpret_cast<const char *>(buffers.agentID),
                  count * sizeof(int));
  if (buffers.agentStrength)
    outFile.write(reinterpret_cast<const char *>(buffers.agentStrength),
                  count * sizeof(float));
  if (buffers.structureType)
    outFile.write(reinterpret_cast<const char *>(buffers.structureType),
                  count * sizeof(uint8_t));

  outFile.close();
  std::cout << "[MAP] Saved world to " << filename << std::endl;
}

bool LoadWorld(WorldBuffers &buffers, const std::string &filename) {
  std::ifstream inFile(filename, std::ios::binary);

  if (!inFile.is_open()) {
    return false;
  }

  // 1. Read Header
  uint32_t count = 0;
  inFile.read(reinterpret_cast<char *>(&count), sizeof(uint32_t));

  if (count != buffers.count) {
    std::cerr << "[ERROR] Map cell count mismatch! Expected " << buffers.count
              << " got " << count << std::endl;
    return false;
  }

  // 2. Load Layers
  if (buffers.height)
    inFile.read(reinterpret_cast<char *>(buffers.height),
                count * sizeof(float));
  if (buffers.temperature)
    inFile.read(reinterpret_cast<char *>(buffers.temperature),
                count * sizeof(float));
  if (buffers.moisture)
    inFile.read(reinterpret_cast<char *>(buffers.moisture),
                count * sizeof(float));
  if (buffers.cultureID)
    inFile.read(reinterpret_cast<char *>(buffers.cultureID),
                count * sizeof(int));
  if (buffers.population)
    inFile.read(reinterpret_cast<char *>(buffers.population),
                count * sizeof(uint32_t));

  // NEW: Critical for Replay
  if (buffers.agentID)
    inFile.read(reinterpret_cast<char *>(buffers.agentID), count * sizeof(int));
  if (buffers.agentStrength)
    inFile.read(reinterpret_cast<char *>(buffers.agentStrength),
                count * sizeof(float));
  if (buffers.structureType)
    inFile.read(reinterpret_cast<char *>(buffers.structureType),
                count * sizeof(uint8_t));

  inFile.close();
  std::cout << "[MAP] Loaded world from " << filename << std::endl;
  return true;
}

} // namespace BinaryExporter
