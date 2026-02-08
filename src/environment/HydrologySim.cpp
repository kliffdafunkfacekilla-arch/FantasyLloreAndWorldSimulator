#include "../../include/Environment.hpp"
#include <cmath>
#include <algorithm>

namespace HydrologySim {

    // ADD 'const WorldSettings& s' here to match the header and main.cpp
    void Update(WorldBuffers& b, const NeighborGraph& g, const WorldSettings& s) {
        if (!b.height || !b.moisture) return;

        // Use settings for logic
        float seaLevel = s.seaLevel; 

        for (int i = 0; i < b.count; ++i) {
            // Skip Ocean logic for hydrology flow
            if (b.height[i] < seaLevel) continue;

            // 1. Simple Moisture Runoff
            // Look for lower neighbor
            int lowestN = -1;
            float minH = b.height[i];
            
            int offset = g.offsetTable[i];
            int count = g.countTable[i];

            for (int k = 0; k < count; ++k) {
                int nIdx = g.neighborData[offset + k];
                if (b.height[nIdx] < minH) {
                    minH = b.height[nIdx];
                    lowestN = nIdx;
                }
            }

            // Move water downhill
            if (lowestN != -1) {
                float flow = b.moisture[i] * 0.1f; // 10% flow per tick
                b.moisture[i] -= flow;
                b.moisture[lowestN] += flow;
                
                // Erosion effect (water carving rivers)
                if (flow > 0.05f) {
                    b.height[i] -= 0.001f;
                    b.height[lowestN] += 0.001f; // Deposit sediment
                }
            }
        }
    }
}
