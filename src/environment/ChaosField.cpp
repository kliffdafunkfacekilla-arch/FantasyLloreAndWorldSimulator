#include "../../include/AssetManager.hpp"
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

void Update(WorldBuffers &b, const NeighborGraph &g, const WorldSettings &s) {
  if (!b.chaos || !g.neighborData)
    return;

  int side = (int)std::sqrt(b.count);
  float angleOffset = s.convergenceAngle;
  int cx = side / 2;
  int cy = side / 2;

  // 112 points along 12 paths converging to center
  for (int p = 0; p < 12; ++p) {
    float pathAngle = (p * (3.14159f * 2.0f) / 12.0f) + angleOffset;
    for (int step = 0; step < 9; ++step) { // ~9 steps per path -> 108 points + 4 extra or center
      float dist = (float)(step + 1) * (float)(side / 2) / 10.0f;
      int px = cx + (int)(cos(pathAngle) * dist);
      int py = cy + (int)(sin(pathAngle) * dist);

      if (px >= 0 && px < side && py >= 0 && py < side) {
        int idx = py * side + px;
        b.chaos[idx] = 1.0f; // Max chaos at source points
      }
    }
  }

  // Convergence point
  b.chaos[cy * side + cx] = 1.0f;
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

    // Mutants spawning
    if (current > 0.8f && b.cultureID[i] == -1 && (rand() % 10000) / 10000.0f < s.mutantSpawnChance) {
      b.cultureID[i] = AssetManager::agentRegistry.size() - 1; // Default mutant is the last one we added
      b.population[i] = 50; // Spawn a pack of mutants
    }
  }

  // Apply back
  std::memcpy(b.chaos, nextChaos.data(), b.count * sizeof(float));
}

} // namespace ChaosField

// Legacy free function (calls namespace version)
void UpdateChaos(WorldBuffers &b, const NeighborGraph &graph,
                 float diffusionRate) {
  (void)diffusionRate; // Use internal rate
  WorldSettings dummySettings; // Legacy compat
  ChaosField::Update(b, graph, dummySettings);
}
