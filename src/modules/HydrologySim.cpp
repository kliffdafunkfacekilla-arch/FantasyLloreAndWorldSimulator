#include "../../include/SimulationModules.hpp"
#include <algorithm>
#include <vector>
#include <iostream>

void HydrologySim::CalculateFlow(WorldBuffers& buffers, const NeighborGraph& graph, uint32_t count) {
    // We use a temporary buffer to store how much water passes through each cell
    std::vector<float> flux(count, 1.0f); // Every cell gets 1.0 initial rain
    
    // To calculate flow in one pass without recursion, we sort cells by height (high to low)
    std::vector<uint32_t> sortedIndices(count);
    for (uint32_t i = 0; i < count; ++i) sortedIndices[i] = i;

    // Use pointers to sort implicitly
    std::sort(sortedIndices.begin(), sortedIndices.end(), [&](uint32_t a, uint32_t b) {
        return buffers.height[a] > buffers.height[b];
    });

    int riverCount = 0;

    // Pass water from high cells to low cells
    for (uint32_t i : sortedIndices) {
        if (buffers.height[i] <= 0.2f) continue; // Stop at "Sea Level"

        int lowestNeighbor = -1;
        float minHeight = buffers.height[i];

        int offset = graph.offsetTable[i];
        uint8_t n_count = graph.countTable[i];

        for (int n = 0; n < n_count; ++n) {
            int neighborID = graph.neighborData[offset + n];
            if (buffers.height[neighborID] < minHeight) {
                minHeight = buffers.height[neighborID];
                lowestNeighbor = neighborID;
            }
        }

        if (lowestNeighbor != -1) {
            flux[lowestNeighbor] += flux[i];
        }
    }

    // Apply results to world state
    for (uint32_t i = 0; i < count; ++i) {
        if (buffers.flux) {
            buffers.flux[i] = flux[i];
        }

        if (flux[i] > 1000.0f) { 
            // This cell is now a Major River
            riverCount++;
        }
    }
    std::cout << "[HYDROLOGY] Flow accumulation complete. Found " << riverCount << " major river segments." << std::endl;
}
