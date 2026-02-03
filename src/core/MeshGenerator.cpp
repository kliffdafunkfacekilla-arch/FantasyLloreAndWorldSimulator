#include "../../include/SimulationModules.hpp"
#include <iostream>
#include <cmath>

// Re-purposing MeshGenerator.cpp to hold graph building logic since it was previously a placeholder
// Alternatively, we could create src/core/GraphBuilder.cpp

void BuildNeighborGraph(WorldBuffers& buffers, uint32_t count, NeighborGraph& graph) {
    std::cout << "[GRAPH] Building connectivity for " << count << " cells...\n";
    
    // Resize tables
    graph.resize(count);
    
    // For this implementation, we will use a simple "radius search" or assume grid-like if unavailable.
    // Since we likely used Poisson, points are scattered.
    // A proper Voronoi/Delaunay is complex. For now, we will implement a spatial hash radius search
    // similar to PointGenerator's validation logic to find neighbors.
    
    // 1. Setup Spatial Grid
    float mapW = std::sqrt((float)count); // Approximation
    float radius = 1.5f * std::sqrt(mapW * mapW / count); // Look slightly beyond avg spacing
    float radiusSq = radius * radius;
    
    // (Simplification: Just clearing data for now. Real implementation requires KD-Tree or Grid)
    // To allow compilation and basic running without crashing:
    graph.neighborData.clear();
    int currentOffset = 0;

    for (uint32_t i = 0; i < count; ++i) {
        graph.offsetTable[i] = currentOffset;
        uint8_t n_count = 0;

        // DUMMY: Self-connection + 1 random for flow testing if we don't do full O(N^2)
        // In production this needs the spatial grid from PointGenerator reused or rebuilt.
        // We will just connect to previous and next index to verify data flow.
        
        // Left Neighbor (index - 1)
        if (i > 0) {
            graph.neighborData.push_back(i - 1);
            n_count++;
        }
        // Right Neighbor (index + 1)
        if (i < count - 1) {
            graph.neighborData.push_back(i + 1);
            n_count++;
        }

        graph.countTable[i] = n_count;
        currentOffset += n_count;
    }
    
    std::cout << "[GRAPH] Built " << graph.neighborData.size() << " connections.\n";
}
