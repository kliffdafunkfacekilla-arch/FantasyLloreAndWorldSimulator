#include "../../include/SimulationModules.hpp"
#include <algorithm>
#include <cstring> // For memcpy
#include <vector>


namespace ChaosField {

struct Rift {
  int index;
  float intensity;
};
std::vector<Rift> activeRifts;

void SpawnRift(WorldBuffers &b, int index, float intensity) {
  if (index < 0 || index >= (int)b.count)
    return;
  activeRifts.push_back({index, intensity});
  if (b.chaos)
    b.chaos[index] = intensity;
}

void ClearRifts() { activeRifts.clear(); }

void Update(WorldBuffers &b, const NeighborGraph &g) {
  if (!b.chaos || !g.neighborData)
    return;

  // 1. EMIT CHAOS (Source)
  for (const auto &rift : activeRifts) {
    if (rift.index >= 0 && rift.index < (int)b.count) {
      b.chaos[rift.index] = rift.intensity;
    }
  }

  // 2. DIFFUSION (Gas simulation)
  static std::vector<float> nextChaos;
  if (nextChaos.size() != b.count)
    nextChaos.resize(b.count);

  float diffusionRate = 0.1f;
  float decayRate = 0.98f; // Magic fades over distance

  for (uint32_t i = 0; i < b.count; ++i) {
    float current = b.chaos[i];

    // Get average of neighbors
    float neighborSum = 0.0f;
    int offset = g.offsetTable[i];
    int count = g.countTable[i];

    for (int k = 0; k < count; ++k) {
      int nIdx = g.neighborData[offset + k];
      neighborSum += b.chaos[nIdx];
    }

    if (count > 0) {
      float avg = neighborSum / count;
      // Move towards average (Diffusion)
      current += (avg - current) * diffusionRate;
    }

    nextChaos[i] = current * decayRate;
  }

  // Apply back
  std::memcpy(b.chaos, nextChaos.data(), b.count * sizeof(float));
}

} // namespace ChaosField

// Legacy free function (calls namespace version)
void UpdateChaos(WorldBuffers &b, const NeighborGraph &graph,
                 float diffusionRate) {
  (void)diffusionRate; // Use internal rate
  ChaosField::Update(b, graph);
}
