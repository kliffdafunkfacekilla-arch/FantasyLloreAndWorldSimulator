#include "../../include/SimulationModules.hpp"
#include <iostream>

// NeighborFinder implementation
// Note: NeighborGraph struct definition is in the header to be shared.

void NeighborFinder::BuildGraph(WorldBuffers& buffers, uint32_t count, NeighborGraph& graph) {
    // Allocate memory for the graph
    // We assume an average of 6-8 neighbors per Voronoi cell
    graph.neighborData = new int[count * 8]; 
    graph.offsetTable = new int[count];
    graph.countTable = new uint8_t[count];

    std::cout << "[GRAPH] Connecting 1,000,000 cells..." << std::endl;

    // TODO: Implement actual Delaunay/Geometry lookup.
    // For now, we create a dummy "Line" graph just to prevent crashes and allow testing.
    // i connects to i-1 and i+1
    
    int currentOffset = 0;
    for (uint32_t i = 0; i < count; ++i) {
        graph.offsetTable[i] = currentOffset;
        uint8_t foundCount = 0;

        // Start Dummy Logic
        if (i > 0) {
            graph.neighborData[currentOffset + foundCount] = i - 1;
            foundCount++;
        }
        if (i < count - 1) {
            graph.neighborData[currentOffset + foundCount] = i + 1;
            foundCount++;
        }
        // End Dummy Logic

        // Update offset for next cell
        currentOffset += foundCount;

        // PERFORMANCE NOTE: 
        // In the full build, we use the Delaunay Triangulation result.
        // For this step, we identify the indices of the 6 closest 
        // points to create the initial adjacency.
        
        // Placeholder for the geometric adjacency logic
        // foundCount = FindNearestNeighbors(i, buffers, ...);

        graph.countTable[i] = foundCount;
    }

    std::cout << "[GRAPH] Adjacency list finalized. Total connections: " << currentOffset << std::endl;
}

void NeighborFinder::Cleanup(NeighborGraph& graph) {
    delete[] graph.neighborData;
    delete[] graph.offsetTable;
    delete[] graph.countTable;
}
