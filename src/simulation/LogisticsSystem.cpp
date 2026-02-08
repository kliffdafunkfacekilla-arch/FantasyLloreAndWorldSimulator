#include "../../include/SimulationModules.hpp"
#include <algorithm>
#include <vector>

namespace LogisticsSystem {

void Update(WorldBuffers &b, const NeighborGraph &g) {
  if (!b.wealth || !b.infrastructure || !g.neighborData)
    return;

  // 1. PRODUCTION (Resources generate based on biomes)
  for (uint32_t i = 0; i < b.count; ++i) {
    float h = b.height[i];
    float t = b.temperature ? b.temperature[i] : 0.5f;
    float m = b.moisture ? b.moisture[i] : 0.5f;
    float pop = b.population ? (float)b.population[i] : 0.0f;

    if (h < 0.4f)
      continue; // Ocean produces nothing (yet)

    float production = 0.0f;

    // Forest (High Moisture, Med Temp) = Wood/Food
    if (m > 0.5f && t > 0.3f)
      production += 1.5f;

    // Mountains (High Height) = Iron/Stone
    if (h > 0.7f)
      production += 2.0f;

    // Plains (Med Height, Med Moisture) = Food
    if (h > 0.4f && h < 0.6f && m > 0.3f)
      production += 1.0f;

    // Add to cell wealth
    b.wealth[i] += production;

    // 2. CONSUMPTION
    // Population eats wealth
    float consumption = pop * 0.01f;
    b.wealth[i] = std::max(0.0f, b.wealth[i] - consumption);

    // 3. INFRASTRUCTURE GROWTH
    // If wealth piles up, build roads/cities
    if (b.wealth[i] > 100.0f) {
      b.infrastructure[i] = std::min(1.0f, b.infrastructure[i] + 0.01f);
      b.wealth[i] -= 10.0f; // Invest wealth into building
    }
  }

  // 4. TRADE (Flow towards Infrastructure)
  // Wealth moves from low-infra to high-infra (Rural -> City)
  static std::vector<float> nextWealth;
  if (nextWealth.size() != b.count)
    nextWealth.resize(b.count);

  for (uint32_t i = 0; i < b.count; ++i) {
    nextWealth[i] = b.wealth[i];
  }

  for (uint32_t i = 0; i < b.count; ++i) {
    float myInfra = b.infrastructure[i];
    if (myInfra <= 0.0f)
      continue;

    // Pull wealth from neighbors
    int offset = g.offsetTable[i];
    int count = g.countTable[i];

    for (int k = 0; k < count; ++k) {
      int nIdx = g.neighborData[offset + k];

      // If neighbor has less infrastructure, pull their wealth
      if (b.infrastructure[nIdx] < myInfra) {
        float transfer = b.wealth[nIdx] * 0.1f; // 10% tax/trade
        nextWealth[nIdx] -= transfer;
        nextWealth[i] += transfer;
      }
    }
  }

  // Copy back
  for (uint32_t i = 0; i < b.count; ++i) {
    b.wealth[i] = std::max(0.0f, nextWealth[i]);
  }
}

} // namespace LogisticsSystem

// Legacy free function
void ProcessLogistics(WorldBuffers &b, uint32_t count) {
  (void)count;
  // Simple fallback without graph
  for (uint32_t i = 1; i < b.count - 1; ++i) {
    if (b.infrastructure[i] > 0.5f) {
      b.population[i] += 10;
    }
  }
}
