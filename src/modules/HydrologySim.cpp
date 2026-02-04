#include "../../include/SimulationModules.hpp"
#include <algorithm>
#include <cmath>
#include <cstring> // For memcpy

namespace HydrologySim {

void Update(WorldBuffers &b, const NeighborGraph &g) {
  if (!b.flux || !b.nextFlux || !b.height || !g.neighborData)
    return;

  // 1. ADD RAINFALL (Write to current flux, then copy to nextFlux as base)
  for (uint32_t i = 0; i < b.count; ++i) {
    float rain = b.moisture[i] * 0.01f; // Small amount per tick
    if (b.height[i] < 0.2f)
      rain = 0.0f; // No rain on ocean (it's already water)
    b.flux[i] += rain;
  }

  // Initialize nextFlux as copy of current flux (before flow modification)
  std::memcpy(b.nextFlux, b.flux, b.count * sizeof(float));

  // 2. CALCULATE FLOW (Downhill) - DOUBLE BUFFERED
  // Read from flux, write changes to nextFlux
  for (uint32_t i = 0; i < b.count; ++i) {
    float h = b.height[i];
    float water = b.flux[i]; // READ from current buffer

    if (water <= 0.0001f)
      continue;
    if (h < 0.2f) {
      b.nextFlux[i] = 0.0f; // Ocean absorbs water
      continue;
    }

    // Find Lowest Neighbor
    int offset = g.offsetTable[i];
    int count = g.countTable[i];

    int lowestIdx = -1;
    float lowestH = h;

    for (int k = 0; k < count; ++k) {
      int nIdx = g.neighborData[offset + k];
      float nH = b.height[nIdx];

      if (nH < lowestH) {
        lowestH = nH;
        lowestIdx = nIdx;
      }
    }

    // If we found a lower spot, move water there
    if (lowestIdx != -1) {
      float drop = h - lowestH;
      // Flow is proportional to slope and amount of water
      float transfer = water * std::min(0.5f, drop * 2.0f);

      b.nextFlux[i] -= transfer;         // WRITE to next buffer
      b.nextFlux[lowestIdx] += transfer; // WRITE to next buffer
    }

    // 3. EVAPORATION
    b.nextFlux[i] *= 0.99f;
  }

  // 4. SWAP BUFFERS (Copy nextFlux back to flux)
  std::memcpy(b.flux, b.nextFlux, b.count * sizeof(float));
}
} // namespace HydrologySim
