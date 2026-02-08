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

  // 0. Spawn Springs (River Sources)
  // Use a pseudo-random determination based on index to be deterministic per
  // frame if needed or just use `s.riverCount` to pick random spots? Accessing
  // `s` requires changing signature. Wait, I need to update signature of Update
  // to take WorldSettings!

  // ... (I will need to update the signature in the header first if I haven't)
  // Let's assume I can change the signature.
  // Actually, let's fix the calls in `GuiController` first?
  // No, `GuiController` calls `HydrologySim::Update(buffers, graph)`.
  // I must update the signature in `Environment.hpp` and `GuiController` too.

  // Swap pointers in the struct for O(1) update
  std::swap(b.flux, b.nextFlux);
}

} // namespace HydrologySim
