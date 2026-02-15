#include "../../include/AssetManager.hpp"
#include "../../include/Biology.hpp"
#include "../../include/Lore.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

namespace AgentSystem {
std::vector<AgentTemplate> speciesRegistry;

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
                       const AgentDefinition &dna) {
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

  // 2. MIGRATION (Animals)
  if (dna.type == AgentType::FAUNA && myPop > 10.0f) {
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

  // Write back
  b.population[i] = (uint32_t)myPop;
}

// Separated Biology System (FAUNA / FLORA)
void UpdateBiology(WorldBuffers &b, const NeighborGraph &g,
                   const WorldSettings &s) {
  if (!b.cultureID || !b.population || !s.enableBiology)
    return;

  for (uint32_t i = 0; i < b.count; ++i) {
    int myID = b.cultureID[i];
    if (myID == -1 || myID >= (int)AssetManager::agentRegistry.size())
      continue;

    const AgentDefinition &dna = AssetManager::agentRegistry[myID];
    if (dna.type == AgentType::FLORA || dna.type == AgentType::FAUNA) {
      ProcessAgentLogic(b, g, i, dna);
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
      ProcessAgentLogic(b, g, i, dna);
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
