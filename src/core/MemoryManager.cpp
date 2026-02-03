#include "../../include/WorldEngine.hpp"
#include <cstring>
#include <iostream>

class MemoryManager {
public:
    void InitializeWorld(const WorldSettings& settings, WorldBuffers& buffers) {
        size_t count = settings.cellCount;

        // 1. Always allocate the Physical Core
        buffers.posX = new float[count];
        buffers.posY = new float[count];
        buffers.height = new float[count];
        std::cout << "[CORE] Allocated " << count << " cells." << std::endl;

        // 2. Optional: Climate
        if (settings.enableClimate) {
            buffers.temperature = new float[count];
            buffers.moisture = new float[count];
            std::cout << "[MODULE] Climate buffers ready." << std::endl;
        }

        // 3. Optional: Chaos
        if (settings.enableChaos) {
            buffers.chaosEnergy = new float[count];
            std::cout << "[MODULE] Chaos field ready." << std::endl;
        }

        // 4. Populate Height (Import or Generate)
        if (!settings.heightmapPath.empty()) {
            // LoadHeightmapData(settings.heightmapPath.c_str(), buffers.height);
        } else {
            // GenerateProceduralTerrain(buffers.height);
        }
    }
};
