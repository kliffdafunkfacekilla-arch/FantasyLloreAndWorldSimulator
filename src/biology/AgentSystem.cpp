#include "../../include/AssetManager.hpp"
#include "../../include/Biology.hpp"
#include "../../include/Lore.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

namespace AgentSystem {
std::vector<AgentTemplate> speciesRegistry;

void SpawnLife(WorldBuffers &b, int count) {
  if (AssetManager::agentRegistry.empty())
    return;

  std::cout << "[SPAWN] Seeding " << count << " life points...\n";
  for (int i = 0; i < count; ++i) {
    int idx = rand() % b.count;
    if (b.height[idx] > 0.3f && b.cultureID[idx] == -1) {
      // Pick a random species from registry
      int randSpecies = rand() % AssetManager::agentRegistry.size();
      const auto &dna = AssetManager::agentRegistry[randSpecies];

      // Simple bioclimatic sanity check (don't spawn polar bears in desert)
      float t = b.temperature[idx];
      float m = b.moisture[idx];
      if (t >= dna.deadlyTempLow && t <= dna.deadlyTempHigh &&
          m >= dna.deadlyMoistureLow && m <= dna.deadlyMoistureHigh) {
        b.cultureID[idx] = dna.id;
        b.population[idx] = (dna.type == AgentType::FLORA) ? 500 : 50;
      }
    }
  }
}

// --- INITIALIZATION ---
void Initialize() {
  // No specific initialization needed for now
}

// --- HELPER: SCORING ---
float CalculateDesire(int cellIdx, const AgentDefinition &dna,
                      const WorldBuffers &b) {
  float temp = b.temperature[cellIdx];
  float moisture = b.moisture[cellIdx];

  // Temperature check
  if (temp < dna.deadlyTempLow || temp > dna.deadlyTempHigh)
    return 0.0f;
  // Moisture check
  if (moisture < dna.deadlyMoistureLow || moisture > dna.deadlyMoistureHigh)
    return 0.0f;

  float tempDiff = std::abs(temp - dna.idealTemp);
  float moistureDiff = std::abs(moisture - dna.idealMoisture);

  float biomeScore = 1.0f - (tempDiff + moistureDiff) * 2.0f;
  biomeScore = std::max(0.0f, biomeScore);

  float foodScore = 0.0f;
  if (dna.type == AgentType::FAUNA) {
    for (auto const &pair : dna.diet) {
      int resID = pair.first;
      float required = pair.second;
      float available = b.GetResource(cellIdx, resID);
      if (available > 1.0f) // Some Threshold
        foodScore += 1.0f;
    }
  }

  float crowding = 0.0f;
  float myPop = (float)b.population[cellIdx];
  if (b.cultureID[cellIdx] == dna.id) {
    if (myPop > 5000.0f)
      crowding = 1.0f;
  }

  return (biomeScore * 2.0f) + foodScore - crowding;
}

// Internal Logic Processor
void ProcessAgentLogic(WorldBuffers &b, const NeighborGraph &g, int i,
                       const AgentDefinition &dna, const ChronosConfig *c = nullptr) {
  int myID = dna.id;
  float myPop = (float)b.population[i];

  // 1. METABOLISM
  bool isStarving = false;
  for (auto const &pair : dna.diet) {
    int resID = pair.first;
    float amount = pair.second;
    float needed = amount * (myPop / 100.0f);
    float available = b.GetResource(i, resID);
    if (available >= needed) {
      b.AddResource(i, resID, -needed);
    } else {
      isStarving = true;
      b.AddResource(i, resID, -available);
    }
  }

  if (!isStarving) {
    for (auto const &pair : dna.output) {
      int resID = pair.first;
      float amount = pair.second;
      float production = amount * (myPop / 100.0f);
      b.AddResource(i, resID, production);
    }

    // Growth
    if (myPop < 10000.0f && rand() % 100 < 10) {
      myPop *= (1.0f + dna.expansionRate);
    }
  } else {
    myPop *= 0.90f; // Starvation death
  }

  // Death from extreme causes
  if (myPop < 1.0f) {
    b.cultureID[i] = -1;
    b.population[i] = 0;
    return;
  }

  // 2. MIGRATION & PREDATION (Animals)
  if (dna.type == AgentType::FAUNA && myPop > 10.0f) {
    // --- WILDLIFE ATTACKS ON CIVILIZATION ---
    bool attacked = false;
    float currentAggression = dna.aggression;
    if (c) {
      // Full moon (0.45 to 0.55) increases aggression significantly
      if (c->moonPhase > 0.45f && c->moonPhase < 0.55f) {
        currentAggression *= 1.5f;
      }
    }
    if (isStarving || currentAggression > 0.5f) { // Hungry or Predator
      int offset = g.offsetTable[i];
      int count = g.countTable[i];
      for (int k = 0; k < count; ++k) {
        int nIdx = g.neighborData[offset + k];
        int nCulture = b.cultureID[nIdx];
        if (nCulture != -1 && nCulture != myID) {
          const AgentDefinition& nDef = AssetManager::agentRegistry[nCulture];
          if (nDef.type == AgentType::CIVILIZED) {
            // Attack civilization
            float attackStrength = myPop * dna.aggression * 0.1f;
            float damage = attackStrength;

            // Defenses
            if (b.defense) damage -= b.defense[nIdx];
            if (damage < 0) damage = 0;

            if (b.population[nIdx] > damage) {
                b.population[nIdx] -= (uint32_t)damage;
            } else {
                b.population[nIdx] = 0;
                b.cultureID[nIdx] = -1; // Wipe them out
            }

            // Fauna feeds on them
            myPop += attackStrength * 0.5f;
            attacked = true;
            break; // only attack one neighbor per tick
          }
        }
      }
    }

    if (!attacked) {
      int bestN = -1;
      float currentScore = CalculateDesire(i, dna, b);
      float bestScore = currentScore;
      int offset = g.offsetTable[i];
      int count = g.countTable[i];

      for (int k = 0; k < count; ++k) {
        int nIdx = g.neighborData[offset + k];
        if (b.height[nIdx] < 0.2f)
          continue; // Ocean
        if (b.cultureID[nIdx] != -1 && b.cultureID[nIdx] != myID)
          continue;

        float s = CalculateDesire(nIdx, dna, b);
        if (s > bestScore) {
          bestScore = s;
          bestN = nIdx;
        }
      }

      if (bestN != -1 && bestScore > currentScore * 1.05f) {
        float migrants = myPop * 0.2f;
        if (b.cultureID[bestN] == -1) {
          b.cultureID[bestN] = myID;
          b.population[bestN] = 0;
        }
        b.population[bestN] += (uint32_t)migrants;
        myPop -= migrants;
      }
    }
  }

  // 3. REPRODUCTION (Plants / Spreads)
  if (dna.type == AgentType::FLORA && myPop > 500.0f) {
    if (rand() % 100 < (dna.expansionRate * 50)) {
      int offset = g.offsetTable[i];
      int count = g.countTable[i];
      int nIdx = g.neighborData[offset + (rand() % count)];

      if (b.cultureID[nIdx] == -1 && CalculateDesire(nIdx, dna, b) > 0.4f) {
        if (b.height[nIdx] > 0.2f) { // Land only
          b.cultureID[nIdx] = myID;
          b.population[nIdx] = 100;
        }
      }
    }
  }

    // --- FARMING & TAMING (CIVILIZED) ---
  if (dna.type == AgentType::CIVILIZED && b.civTier && b.civTier[i] >= 1) {
    int offset = g.offsetTable[i];
    int count = g.countTable[i];
    for (int k = 0; k < count; ++k) {
      int nIdx = g.neighborData[offset + k];
      int nCulture = b.cultureID[nIdx];
      if (nCulture != -1 && nCulture != myID) {
        const AgentDefinition& nDef = AssetManager::agentRegistry[nCulture];
        if (nDef.type == AgentType::FLORA && nDef.isFarmable) {
          // Farm the flora, gaining resources (food: ID 0)
          b.AddResource(i, 0, 5.0f);
        } else if (nDef.type == AgentType::FAUNA && nDef.isDomesticatable) {
          // Tame the fauna, gaining resources and strength
          b.AddResource(i, 0, 2.0f);
          b.agentStrength[i] += 1.0f;
        }
      }
    }
  }

  // Write back
  b.population[i] = (uint32_t)myPop;
}

// Separated Biology System (FAUNA / FLORA)
void UpdateBiology(WorldBuffers &b, const NeighborGraph &g,
                   const WorldSettings &s, const ChronosConfig &c) {
  if (!b.cultureID || !b.population || !s.enableBiology)
    return;

  for (uint32_t i = 0; i < b.count; ++i) {
    int myID = b.cultureID[i];
    if (myID == -1 || myID >= (int)AssetManager::agentRegistry.size())
      continue;

    const AgentDefinition &dna = AssetManager::agentRegistry[myID];
    if (dna.type == AgentType::FLORA || dna.type == AgentType::FAUNA) {
      ProcessAgentLogic(b, g, i, dna, &c);
    }
  }
}

// Correct Signature Wrapper for Civilization logic
void UpdateCivilization(WorldBuffers &b, const NeighborGraph &g) {
  if (!b.cultureID || !b.population)
    return;

  for (uint32_t i = 0; i < b.count; ++i) {
    int myID = b.cultureID[i];
    if (myID == -1 || myID >= (int)AssetManager::agentRegistry.size())
      continue;

    const AgentDefinition &dna = AssetManager::agentRegistry[myID];
    if (dna.type == AgentType::CIVILIZED) {
      ProcessAgentLogic(b, g, i, dna, nullptr);
      // CivilizationSim handles construction and age-related death elsewhere
      // (CivilizationSim::Update)
    }
  }
}

// --- UTILS ---
void SpawnCivilization(WorldBuffers &b, int count) {
  int civID = -1;
  for (const auto &a : AssetManager::agentRegistry) {
    if (a.type == AgentType::CIVILIZED) {
      civID = a.id;
      break;
    }
  }
  if (civID == -1)
    return;

  for (int i = 0; i < count; ++i) {
    int idx = rand() % b.count;
    if (b.height[idx] > 0.2f && b.cultureID[idx] == -1) {
      b.cultureID[idx] = civID;
      b.population[idx] = 1000;
      b.civTier[idx] = 1;
      LoreScribeNS::LogEvent(0, "SPAWN", idx, "A new civilization appears.");
    }
  }
}

} // namespace AgentSystem
