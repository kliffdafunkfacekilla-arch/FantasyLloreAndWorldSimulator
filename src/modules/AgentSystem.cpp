#include "../../include/SimulationModules.hpp"
#include "../../include/WorldEngine.hpp"
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>


namespace AgentSystem {

// Registry of all species in the world
std::vector<AgentTemplate> speciesRegistry;

void Initialize() {
  if (!speciesRegistry.empty())
    return;

  // 1. Create Basic "Human" Template
  AgentTemplate human;
  human.id = 0;
  std::strncpy(human.name, "Human", 31);
  human.isStatic = false;
  human.canBuild = true;
  human.canWar = true;
  human.aggression = 0.5f;
  human.intelligence = 0.8f;
  human.sociality = 0.9f;
  human.idealTemp = 0.5f;
  human.adaptiveRate = 0.01f;
  human.resourceNeeds[0] = 1.0f;
  human.resourceNeeds[1] = 0.5f;
  human.resourceNeeds[2] = 0.2f;
  human.resourceNeeds[3] = 0.0f;

  // 2. Create "Pine Trees" (Static, loves cold)
  AgentTemplate pine;
  pine.id = 1;
  std::strncpy(pine.name, "Pine", 31);
  pine.isStatic = true;
  pine.canBuild = false;
  pine.canWar = false;
  pine.aggression = 0.0f;
  pine.intelligence = 0.0f;
  pine.sociality = 0.0f;
  pine.idealTemp = 0.2f;
  pine.adaptiveRate = 0.05f;
  pine.resourceNeeds[0] = 0.1f;
  pine.resourceNeeds[1] = 0.0f;
  pine.resourceNeeds[2] = 0.0f;
  pine.resourceNeeds[3] = 0.0f;

  // 3. Create "Wolves" (Mobile, Aggressive)
  AgentTemplate wolf;
  wolf.id = 2;
  std::strncpy(wolf.name, "Wolf", 31);
  wolf.isStatic = false;
  wolf.canBuild = false;
  wolf.canWar = true;
  wolf.aggression = 0.9f;
  wolf.intelligence = 0.4f;
  wolf.sociality = 0.6f;
  wolf.idealTemp = 0.3f;
  wolf.adaptiveRate = 0.1f;
  wolf.resourceNeeds[0] = 1.0f;
  wolf.resourceNeeds[1] = 0.0f;
  wolf.resourceNeeds[2] = 0.0f;
  wolf.resourceNeeds[3] = 0.0f;

  speciesRegistry.push_back(human);
  speciesRegistry.push_back(pine);
  speciesRegistry.push_back(wolf);

  std::cout << "[AGENTS] Species Registry initialized with "
            << speciesRegistry.size() << " templates.\n";
}

// Handle Wildlife Evolution and Vegetation Growth
void UpdateBiology(WorldBuffers &b, const WorldSettings &s) {
  if (!s.enableBiology)
    return;
  if (!b.population || !b.temperature || !b.moisture)
    return;

  for (uint32_t i = 0; i < b.count; ++i) {
    float temp = b.temperature[i];
    float rain = b.moisture[i];
    float h = b.height[i];

    if (h < s.seaLevel)
      continue; // Skip ocean

    // Check if suitable for Trees (ID 1)
    // Trees like moisture > 0.5 and moderate temp
    if (rain > 0.5f && temp > 0.2f && temp < 0.8f) {
      // If population low, grow trees
      if (b.population[i] < 1000) {
        b.population[i] += 10 * s.timeScale; // Growth
      }
    }
    // Desert scrub - harsh conditions
    else if (temp > 0.8f && rain < 0.2f) {
      // Harsh conditions kill population
      b.population[i] = (uint32_t)(b.population[i] * 0.98f);
    }
  }
}

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
    uint32_t currentPop = b.population[idx];

    // CRITERIA:
    // 1. Above Sea Level (Land)
    // 2. Not too high (Not peaks)
    // 3. Has Moisture (Habitable)
    // 4. Has Fresh Water (Flux > 0.5) OR is Coastal
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
