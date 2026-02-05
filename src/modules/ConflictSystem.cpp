#include "../../include/SimulationModules.hpp"
#include "../../include/WorldEngine.hpp"

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

    // Check neighbors
    int offset = g.offsetTable[i];
    int count = g.countTable[i];

    for (int k = 0; k < count; ++k) {
      int nIdx = g.neighborData[offset + k];
      int nFaction = b.factionID[nIdx];

      // If neighbor is different faction (or empty)
      if (nFaction != myFaction) {
        float nPop = (float)b.population[nIdx];

        // --- BATTLE FORMULA ---
        // Attack: My Pop * Aggression (Assume 1.0 for now)
        // Defense: Their Pop * Terrain Bonus (Mountains harder to take)

        float attackScore =
            myPop * 1.0f; // Add Tech/Aggression multipliers later
        float terrainDef =
            1.0f + (b.height[nIdx] * 2.0f); // Height gives defense
        float defenseScore = nPop * terrainDef;

        // INVASION SUCCESS
        if (attackScore > defenseScore * 1.5f) { // Need 1.5x advantage
          // 1. Log Event (Lore) if taking a city
          if (nPop > 1000) {
            LoreScribeNS::LogEvent(0, "CONQUEST", nIdx,
                                   "Territory conquered by faction " +
                                       std::to_string(myFaction));
          }

          // 2. Casualties
          b.population[i] -= (uint32_t)(nPop * 0.1f); // Attackers lose men

          // 3. Conquest
          b.factionID[nIdx] = myFaction;
          b.population[nIdx] = (uint32_t)(myPop * 0.2f); // Move some people in

          // 4. Chaos from war
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
