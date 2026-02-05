#include "../../include/SimulationModules.hpp"
#include "../../include/WorldEngine.hpp"
#include <cmath>

namespace ConflictSystem {

void Update(WorldBuffers &b, const NeighborGraph &g, const WorldSettings &s) {
  if (!b.factionID || !b.population || !g.neighborData)
    return;
  if (!s.enableConflict)
    return;

  // Iterate all cells
  for (uint32_t i = 0; i < b.count; ++i) {
    int myFaction = b.factionID[i];
    if (myFaction == 0)
      continue; // Empty land doesn't attack

    float myPop = (float)b.population[i];
    if (myPop < 100.0f)
      continue; // Too weak to expand

    // --- ECONOMIC WARFARE: Wealth buys power ---
    float wealth = (b.wealth) ? b.wealth[i] : 0.0f;
    float myInfra = (b.infrastructure) ? b.infrastructure[i] : 0.0f;
    
    // Rich nations can fund larger attacks
    float attackPower = myPop * (1.0f + (wealth * 0.01f));

    // Check neighbors
    int offset = g.offsetTable[i];
    int count = g.countTable[i];

    for (int k = 0; k < count; ++k) {
      int nIdx = g.neighborData[offset + k];
      int nFaction = b.factionID[nIdx];

      // If neighbor is different faction (or empty)
      if (nFaction != myFaction) {
        float nPop = (float)b.population[nIdx];
        float nInfra = (b.infrastructure) ? b.infrastructure[nIdx] : 0.0f;

        // --- DEFENSE SCORING ---
        // Infrastructure (Walls) multiply defense massively
        float defenseScore = nPop * (1.0f + (nInfra * 5.0f));
        
        // Terrain Defense: Mountains = 3x defense
        if (b.height[nIdx] > 0.7f) defenseScore *= 3.0f;
        // Hills give moderate bonus
        else if (b.height[nIdx] > 0.55f) defenseScore *= 1.5f;

        // INVASION SUCCESS (Need 1.5x advantage)
        if (attackPower > defenseScore * 1.5f) {
          // 1. Log Event (Lore) if taking a city
          if (nPop > 1000 && nInfra > 0.5f) {
            LoreScribeNS::LogEvent(0, "CONQUEST", nIdx,
                                   "City conquered by faction " +
                                       std::to_string(myFaction));
          }

          // 2. Casualties - Attackers lose men
          b.population[i] -= (uint32_t)(nPop * 0.1f);

          // 3. Looting - Steal 50% of wealth
          if (b.wealth) {
            float loot = b.wealth[nIdx] * 0.5f;
            b.wealth[i] += loot;
            b.wealth[nIdx] -= loot;
          }

          // 4. War Damage - Destroy 50% of infrastructure
          if (b.infrastructure) {
            b.infrastructure[nIdx] *= 0.5f;
          }

          // 5. Conquest - Transfer territory
          b.factionID[nIdx] = myFaction;
          b.population[nIdx] = (uint32_t)(myPop * 0.2f); // Move some people in

          // 6. Chaos from war
          if (b.chaos)
            b.chaos[nIdx] += 0.2f;
        }
      }
    }
  }
}

} // namespace ConflictSystem

// Legacy free function implementation (calls the namespace version)
void ResolveConflicts(WorldBuffers &b, const NeighborGraph &graph) {
  WorldSettings defaultSettings;
  ConflictSystem::Update(b, graph, defaultSettings);
}
