#include "../../include/SimulationModules.hpp"
#include <algorithm>
#include <cmath>
#include <vector>


namespace HydrologySim {

void Update(WorldBuffers &b, const NeighborGraph &g) {
  if (!b.flux || !b.height || !g.neighborData)
    return;

  // 1. ADD RAINFALL
  // Rain falls based on Moisture map
  for (uint32_t i = 0; i < b.count; ++i) {
    float rain = b.moisture[i] * 0.01f; // Small amount per tick
    if (b.height[i] < 0.2f)
      rain = 0.0f; // No rain on ocean (it's already water)
    b.flux[i] += rain;
  }

  // 2. CALCULATE FLOW (Downhill)
  // We need a temporary buffer to store the *changes* so we don't handle
  // the same water multiple times in one sweep (unless we want
  // order-dependency). For simplicity/speed, we'll do in-place but iterate
  // carefully or accept some bias. Better: Use a "Mass Transfer" approach.

  // Loop through all cells
  for (uint32_t i = 0; i < b.count; ++i) {
    float h = b.height[i];
    float water = b.flux[i];

    if (water <= 0.0001f)
      continue;
    if (h < 0.2f) {
      b.flux[i] = 0.0f; // Ocean absorbs water
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

      // Water fills pits, so effective height = height + water level?
      // For now, simple terrain height flow
      if (nH < lowestH) {
        lowestH = nH;
        lowestIdx = nIdx;
      }
    }

    // If we found a lower spot, move water there
    if (lowestIdx != -1) {
      float drop = h - lowestH;
      // Flow is proportional to slope (drop) and amount of water
      // But don't move *all* of it in one tick, or it teleports.
      float transfer = water * 0.5f;

      b.flux[i] -= transfer;
      b.flux[lowestIdx] += transfer;
    }

    // 3. EVAPORATION
    b.flux[i] *= 0.99f;
  }
}
} // namespace HydrologySim
