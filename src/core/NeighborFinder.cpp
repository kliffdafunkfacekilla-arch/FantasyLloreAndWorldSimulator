#include "../../include/SimulationModules.hpp"
#include <cmath>
#include <iostream>


// NeighborFinder implementation
// Note: NeighborGraph struct definition is in the header to be shared.

void NeighborFinder::BuildGraph(WorldBuffers &buffers, uint32_t count,
                                NeighborGraph &graph) {
  // Allocate memory for the graph
  // We assume an average of 6-8 neighbors per Voronoi cell
  graph.neighborData = new int[count * 8];
  graph.offsetTable = new int[count];
  graph.countTable = new uint8_t[count];

  std::cout << "[GRAPH] Connecting 1,000,000 cells..." << std::endl;

  // TODO: Implement actual Delaunay/Geometry lookup.
  // For now, we create a dummy "Line" graph just to prevent crashes and allow
  // testing. i connects to i-1 and i+1

  int side = (int)std::sqrt(count);
  int currentOffset = 0;

  std::cout << "[GRAPH] Grid Dimensions: " << side << "x" << side << "\n";

  for (uint32_t i = 0; i < count; ++i) {
    graph.offsetTable[i] = currentOffset;
    uint8_t foundCount = 0;

    // Grid Logic: Convert Index -> (X, Y)
    int x = i % side;
    int y = i / side;

    // Check all 8 neighbors
    for (int dy = -1; dy <= 1; ++dy) {
      for (int dx = -1; dx <= 1; ++dx) {
        // Skip self
        if (dx == 0 && dy == 0)
          continue;

        int nx = x + dx;
        int ny = y + dy;

        // Bounds Check
        if (nx >= 0 && nx < side && ny >= 0 && ny < side) {
          int neighborIdx = ny * side + nx;
          graph.neighborData[currentOffset + foundCount] = neighborIdx;
          foundCount++;
        }
      }
    }

    graph.countTable[i] = foundCount;
    currentOffset += foundCount;
  }

  std::cout << "[GRAPH] Adjacency list finalized. Total connections: "
            << currentOffset << std::endl;
}

void NeighborFinder::Cleanup(NeighborGraph &graph) {
  delete[] graph.neighborData;
  delete[] graph.offsetTable;
  delete[] graph.countTable;
}
