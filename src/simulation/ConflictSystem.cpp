#include "../../include/AssetManager.hpp"
#include "../../include/SimulationModules.hpp"
#include "../../include/WorldEngine.hpp"
#include <algorithm>
#include <cmath>
#include <vector>

namespace ConflictSystem {

// Helper: Calculate how much a faction wants a specific tile
float CalculateDesire(int cellIndex, int speciesID, const WorldBuffers &b) {
  // Get species DNA (fallback to ID 0 if invalid)
  if (AssetManager::speciesRegistry.empty())
    return 0.5f;

  const AgentTemplate &species =
      (speciesID < (int)AssetManager::speciesRegistry.size())
          ? AssetManager::speciesRegistry[speciesID]
          : AssetManager::speciesRegistry[0];

  // 1. Biome Fit (Temperature)
  float temp = (b.temperature) ? b.temperature[cellIndex] : 0.5f;
  float ideal = species.idealTemp;
  float biomeFit = 1.0f - std::abs(temp - ideal);

  // 2. Resource Attraction
  float resourceBonus = 0.0f;
  float m = (b.moisture) ? b.moisture[cellIndex] : 0.5f;
  float h = b.height[cellIndex];

  // Forest = Food/Wood, Mountain = Iron/Gold
  if (m > 0.6f && h > 0.4f)
    resourceBonus += 0.4f; // Forest
  if (h > 0.7f)
    resourceBonus += 0.6f; // Mountains

  // 3. Chaos Penalty
  float chaos = (b.chaos) ? b.chaos[cellIndex] : 0.0f;
  float magicPenalty = (chaos > 0.2f) ? -chaos : 0.0f;

  return std::max(0.0f,
                  std::min(2.0f, biomeFit + resourceBonus + magicPenalty));
}

void Update(WorldBuffers &b, const NeighborGraph &g, const WorldSettings &s) {
  if (!b.factionID || !b.population || !g.neighborData)
    return;
  if (!s.enableConflict)
    return;

  // Double buffer to prevent instant map-crossing
  std::vector<int> nextFactionID(b.count);
  for (uint32_t i = 0; i < b.count; ++i) {
    nextFactionID[i] = b.factionID[i];
  }

  float seaLevel = s.seaLevel;
  float expansionCost = 20.0f;
  float expansionThreshold = 0.5f;

  for (uint32_t i = 0; i < b.count; ++i) {
    // OCEAN FIX: Skip ocean cells entirely
    if (b.height[i] < seaLevel)
      continue;

    int myFaction = b.factionID[i];
    if (myFaction == 0)
      continue;

    float myPop = (float)b.population[i];
    if (myPop < 100.0f)
      continue;

    // WEALTH CHECK: Need money to expand
    float wealth = (b.wealth) ? b.wealth[i] : 0.0f;
    if (wealth < expansionCost)
      continue;

    float myInfra = (b.infrastructure) ? b.infrastructure[i] : 0.0f;
    float attackPower = myPop * (1.0f + (wealth * 0.01f));

    // Species-based decision making
    int speciesID = (myFaction - 1) %
                    std::max(1, (int)AssetManager::speciesRegistry.size());

    // Find best neighbor to expand into
    int bestTarget = -1;
    float bestScore = 0.0f;

    int offset = g.offsetTable[i];
    int count = g.countTable[i];

    for (int k = 0; k < count; ++k) {
      int nIdx = g.neighborData[offset + k];

      // OCEAN FIX: Don't cross water
      if (b.height[nIdx] < seaLevel)
        continue;

      int nFaction = b.factionID[nIdx];
      if (nFaction == myFaction)
        continue;

      // Calculate desire score
      float score = CalculateDesire(nIdx, speciesID, b);

      // Reduce score for occupied territories
      if (nFaction != 0) {
        float enemyStr = (float)b.population[nIdx];
        if (enemyStr > myPop)
          score *= 0.1f;
      }

      if (score > bestScore) {
        bestScore = score;
        bestTarget = nIdx;
      }
    }

    // Execute expansion if score is good enough
    if (bestTarget != -1 && bestScore > expansionThreshold) {
      int nIdx = bestTarget;
      int nFaction = b.factionID[nIdx];
      float nPop = (float)b.population[nIdx];
      float nInfra = (b.infrastructure) ? b.infrastructure[nIdx] : 0.0f;

      // EMPTY LAND: Settle it
      if (nFaction == 0) {
        nextFactionID[nIdx] = myFaction;
        b.population[nIdx] = 10;
        if (b.wealth)
          b.wealth[i] -= expansionCost;
      }
      // ENEMY: Fight for it
      else {
        float defenseScore = nPop * (1.0f + (nInfra * 5.0f));
        if (b.height[nIdx] > 0.7f)
          defenseScore *= 3.0f;
        else if (b.height[nIdx] > 0.55f)
          defenseScore *= 1.5f;

        // --- CIVIL WAR BONUS ---
        // Same culture fighting = 50% attack bonus (internal knowledge)
        if (b.cultureID) {
          int myCulture = b.cultureID[i];
          int enemyCulture = b.cultureID[nIdx];
          if (myCulture == enemyCulture && myCulture > 0) {
            attackPower *= 1.5f;
          }
        }

        if (attackPower > defenseScore * 1.5f) {
          // Lore event for city conquest
          if (nPop > 1000 && nInfra > 0.5f) {
            LoreScribeNS::LogEvent(0, "CONQUEST", nIdx,
                                   "City conquered by faction " +
                                       std::to_string(myFaction));
          }

          // Casualties
          b.population[i] -= (uint32_t)(nPop * 0.1f);

          // Looting
          if (b.wealth) {
            float loot = b.wealth[nIdx] * 0.5f;
            b.wealth[i] += loot;
            b.wealth[nIdx] -= loot;
          }

          // War damage
          if (b.infrastructure) {
            b.infrastructure[nIdx] *= 0.5f;
          }

          // Conquest
          nextFactionID[nIdx] = myFaction;
          b.population[nIdx] = (uint32_t)(myPop * 0.2f);

          // --- CULTURE SYSTEM ---
          // Genocide vs Assimilation based on attacker aggression
          if (b.cultureID) {
            int myCulture = b.cultureID[i];
            int enemyCulture = b.cultureID[nIdx];

            // Get faction aggression (default 0.5)
            float aggression = 0.5f;
            if (myFaction > 0 &&
                myFaction <= (int)AssetManager::factionRegistry.size()) {
              aggression =
                  AssetManager::factionRegistry[myFaction - 1].aggression;
            }

            if (aggression > 0.7f) {
              // GENOCIDE: Replace their culture
              b.cultureID[nIdx] = myCulture;
              LoreScribeNS::LogEvent(0, "GENOCIDE", nIdx,
                                     "Population replaced by conquerors");
            } else {
              // ASSIMILATION: Keep defender's culture (multi-ethnic empire)
              // Culture stays as enemyCulture
            }
          }

          // Chaos from war
          if (b.chaos)
            b.chaos[nIdx] += 0.2f;
        }
      }
    }
  }

  // Apply buffered changes
  for (uint32_t i = 0; i < b.count; ++i) {
    b.factionID[i] = nextFactionID[i];
  }

  // --- BANDIT SPAWN LOGIC ---
  // Rich but undefended settlements attract raiders
  if (rand() % 1000 < 5) { // 0.5% chance per tick
    int idx = rand() % b.count;

    float wealth = (b.wealth) ? b.wealth[idx] : 0.0f;
    float defense = (b.defense) ? b.defense[idx] : 0.0f;

    if (wealth > 200.0f && defense < 5.0f && b.population[idx] > 100) {
      // BANDITS RAID!
      float stolen = wealth * 0.3f;
      b.wealth[idx] -= stolen;

      // Population loss
      b.population[idx] = (uint32_t)(b.population[idx] * 0.8f);

      // Increase chaos
      if (b.chaos)
        b.chaos[idx] += 0.15f;

      LoreScribeNS::LogEvent(0, "RAID", idx,
                             "Bandits looted an undefended settlement");
    }
  }
}

} // namespace ConflictSystem

// Legacy free function
void ResolveConflicts(WorldBuffers &b, const NeighborGraph &graph) {
  WorldSettings defaultSettings;
  ConflictSystem::Update(b, graph, defaultSettings);
}
