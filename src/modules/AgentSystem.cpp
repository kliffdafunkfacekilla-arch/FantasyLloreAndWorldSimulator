#include "../../include/SimulationModules.hpp"
#include <cstdlib>
#include <iostream>


namespace AgentSystem {

void SpawnCivilization(WorldBuffers &b, int factionId) {
  if (!b.population || !b.factionID)
    return;

  std::cout << "[AGENTS] Attempting to spawn Faction " << factionId << "...\n";

  int spawnedCount = 0;
  int attempts = 0;

  // Try to spawn 5 Initial Settlements
  while (spawnedCount < 5 && attempts < 1000) {
    int idx = rand() % b.count;
    attempts++;

    float h = b.height[idx];
    float m = b.moisture ? b.moisture[idx] : 0.5f;
    float f = b.flux ? b.flux[idx] : 0.0f;
    int currentPop = b.population[idx];

    // CRITERIA:
    // 1. Above Sea Level (Land)
    // 2. Not too high (Not peaks)
    // 3. Has Moisture (Habitable)
    // 4. Has Fresh Water (Flux > 0.5) OR is Coastal (Logic tricky without
    // adjacency here, rely on flux)
    // 5. Empty

    if (h > 0.22f && h < 0.8f && m > 0.3f && f > 1.0f && currentPop == 0) {
      b.population[idx] = 100; // Village Start
      b.factionID[idx] = factionId;
      b.infrastructure[idx] = 1.0f; // Basic settlement
      spawnedCount++;
    }
  }
  std::cout << "[AGENTS] Spawned " << spawnedCount << " settlements.\n";
}

void UpdateCivilization(WorldBuffers &b, const NeighborGraph &g) {
  if (!b.population || !g.neighborData)
    return;

  // Iterate all cells
  for (uint32_t i = 0; i < b.count; ++i) {
    if (b.population[i] > 0) {

      // 1. GROWTH
      // Simple logistic growth
      // Carrying capacity ~ 10000
      float growthRate = 0.01f; // 1% per tick
      if (b.population[i] < 10000) {
        b.population[i] += (uint32_t)(b.population[i] * growthRate);
      }

      // 2. MIGRATION / EXPANSION
      // If overcrowded, move to neighbor
      if (b.population[i] > 1000) {

        int offset = g.offsetTable[i];
        int count = g.countTable[i];

        // Try a few random neighbors
        for (int tryN = 0; tryN < 3; ++tryN) {
          int k = rand() % count;
          int nIdx = g.neighborData[offset + k];

          // Check if valid land
          if (b.height[nIdx] > 0.2f && b.factionID[nIdx] == 0) {
            // Colonize!
            b.population[nIdx] = 100;
            b.factionID[nIdx] = b.factionID[i];
            b.population[i] -= 100; // Cost of settler
            break;                  // Done for this tick
          }
        }
      }
    }
  }
}
} // namespace AgentSystem
