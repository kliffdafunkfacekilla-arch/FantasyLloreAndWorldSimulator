#include "../../include/SimulationModules.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace HydrologySim {

void Update(WorldBuffers &b, const NeighborGraph &g) {
  if (!b.flux || !b.nextFlux || !b.height || !g.neighborData)
    return;

  // Clear write buffer for this tick
  std::fill_n(b.nextFlux, b.count, 0.0f);

  for (uint32_t i = 0; i < b.count; ++i) {
    float h = b.height[i];
    float water = b.flux[i];

    // 1. Add Rainfall
    float rain = b.moisture[i] * 0.005f; // Moisture-based rain
    if (h < 0.2f)
      rain = 0.0f;
    float currentWater = water + rain;

    if (currentWater < 0.0001f)
      continue;
    if (h < 0.2f) {
      b.nextFlux[i] = 0.0f; // Ocean consumption
      continue;
    }

    // 2. Find Lowest Neighbor
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

    // 3. Move Water
    if (lowestIdx != -1) {
      float diff = h - lowestH;
      float transfer = currentWater * std::min(0.8f, diff * 5.0f);
      b.nextFlux[i] += (currentWater - transfer);
      b.nextFlux[lowestIdx] += transfer;
    } else {
      b.nextFlux[i] += currentWater; // Trapped (Puddle)
    }
  }

  // 4. Evaporation & Swap
  for (uint32_t i = 0; i < b.count; ++i) {
    b.nextFlux[i] *= 0.99f;
  }

  // Swap pointers in the struct for O(1) update
  std::swap(b.flux, b.nextFlux);
}

} // namespace HydrologySim
