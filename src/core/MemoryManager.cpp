#include "../../include/WorldEngine.hpp"
#include <cstring>
#include <iostream>

class MemoryManager {
public:
    void InitializeWorld(const WorldSettings& settings, WorldBuffers& buffers) {
        size_t count = settings.cellCount;
        buffers.count = count; // Ensure renderers know the size

        // 1. Core
        buffers.posX = new float[count];
        buffers.posY = new float[count];
        buffers.height = new float[count];
        std::cout << "[CORE] Allocated " << count << " cells." << std::endl;

        // 2. Climate
        if (settings.enableClimate) {
            buffers.temperature = new float[count];
            buffers.moisture = new float[count];
            buffers.windDX = new float[count];
            buffers.windStrength = new float[count];
            buffers.windDY = new float[count]; // Added DY for wind visualization if needed
            std::memset(buffers.windDY, 0, count * sizeof(float)); 
            std::cout << "[MODULE] Climate & Wind buffers ready." << std::endl;
        }

        // 3. Hydrology
        if (settings.enableHydrology) {
            buffers.flux = new float[count];
            std::cout << "[MODULE] Hydrology flux buffer ready." << std::endl;
        }
        
        // 4. Life & Civilization
        if (settings.enableFactions || settings.resourceCount > 0) {
             buffers.population = new uint32_t[count];
             buffers.speciesID = new int[count];
             buffers.factionID = new int[count];
             buffers.infrastructure = new float[count];

             // Initialize
             std::memset(buffers.population, 0, count * sizeof(uint32_t));
             std::memset(buffers.speciesID, -1, count * sizeof(int));
             std::memset(buffers.factionID, 0, count * sizeof(int)); 
             std::memset(buffers.infrastructure, 0, count * sizeof(float));

             if (settings.resourceCount > 0) {
                 buffers.resourceLevels = new float[count * settings.resourceCount];
                 std::memset(buffers.resourceLevels, 0, count * settings.resourceCount * sizeof(float));
             }
             std::cout << "[MODULE] Life & Civilization ready.\n";
        }

        // 5. Chaos
        if (settings.enableChaos) {
            buffers.chaosEnergy = new float[count];
            std::cout << "[MODULE] Chaos field ready." << std::endl;
        }
    }
};
