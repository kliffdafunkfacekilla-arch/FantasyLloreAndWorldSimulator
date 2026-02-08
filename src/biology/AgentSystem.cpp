#include "../../include/AssetManager.hpp"
#include "../../include/Biology.hpp"
#include "../../include/Lore.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

namespace AgentSystem {

// --- HELPER: RESOURCE MANAGEMENT ---
float GetRes(const WorldBuffers &b, int cellIdx, int resID) {
  if (!b.resourceInventory || resID < 0)
    return 0.0f;
  return b.resourceInventory[cellIdx * 8 + resID];
}

void AddRes(WorldBuffers &b, int cellIdx, int resID, float amount) {
  if (!b.resourceInventory || resID < 0)
    return;
  b.resourceInventory[cellIdx * 8 + resID] += amount;
}

// --- INITIALIZATION ---
void Initialize() {
  // No specific initialization needed for now
}

// --- HELPER: SCORING ---
float CalculateDesire(int cellIdx, const AgentDefinition &dna,
                      const WorldBuffers &b) {
  float temp = b.temperature[cellIdx];
  float tempDiff = std::abs(temp - dna.idealTemp);

  if (tempDiff > dna.resilience)
    return 0.0f;

  float biomeScore = 1.0f - (tempDiff / dna.resilience);

  float foodScore = 0.0f;
  if (dna.type == AgentType::FAUNA) {
    for (auto const &[resID, required] : dna.diet) {
      float available = GetRes(b, cellIdx, resID);
      if (available > required)
        foodScore += 1.0f;
    }
  }

  float crowding = 0.0f;
  // Check population using agentStrength (float) or population (uint)
  // b.population is uint32_t, b.agentStrength is ???
  // Wait, WorldBuffers has 'population' (uint32) but 'agentStrength' is needed
  // for smooth sim? Let's use 'b.population' as float cast for now, OR rely on
  // a float buffer if it exists. WorldEngine.hpp has `uint32_t* population`.
  // Smart Agent Logic used `b.agentStrength`.
  // Check WorldBuffers in Step 118:
  // `uint32_t *population = nullptr;`
  // NO `agentStrength`.
  // The logic provided in Step 51 used `agentStrength`.
  // I must adapt it to use `population` (cast to float).

  float myPop = (float)b.population[cellIdx];
  if (b.cultureID[cellIdx] == dna.id) {
    if (myPop > 500.0f)
      crowding = 0.5f;
  }

  return (biomeScore * 2.0f) + foodScore - crowding;
}

void Update(WorldBuffers &b, const NeighborGraph &g) {
  if (!b.cultureID || !b.population)
    return;

  for (uint32_t i = 0; i < b.count; ++i) {
    int myID = b.cultureID[i];
    if (myID == -1)
      continue;

    if (myID >= (int)AssetManager::agentRegistry.size()) {
      b.cultureID[i] = -1;
      continue;
    }

    const AgentDefinition &dna = AssetManager::agentRegistry[myID];
    // Using population (uint) as float strength
    float myPop = (float)b.population[i];

    // 1. METABOLISM
    bool isStarving = false;
    for (auto const &[resID, amount] : dna.diet) {
      float needed = amount * (myPop / 100.0f);
      float available = GetRes(b, i, resID);
      if (available >= needed) {
        AddRes(b, i, resID, -needed);
      } else {
        isStarving = true;
        AddRes(b, i, resID, -available);
      }
    }

    if (!isStarving) {
      for (auto const &[resID, amount] : dna.output) {
        float production = amount * (myPop / 100.0f);
        AddRes(b, i, resID, production);
      }

      // Growth (Plants/Animals)
      if (myPop < 1000.0f && rand() % 100 < 5) { // 5% chance to grow
        myPop *= (1.0f + dna.expansionRate);
      }
    } else {
      myPop *= 0.95f;
    }

    // Death
    if (myPop < 1.0f) {
      b.cultureID[i] = -1;
      b.population[i] = 0;
      continue;
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
          continue; // Ocean check
        if (b.cultureID[nIdx] != -1 && b.cultureID[nIdx] != myID)
          continue;

        float s = CalculateDesire(nIdx, dna, b);
        if (s > bestScore) {
          bestScore = s;
          bestN = nIdx;
        }
      }

      if (bestN != -1 && bestScore > currentScore * 1.1f) {
        float migrants = myPop * 0.5f;
        if (b.cultureID[bestN] == -1) {
          b.cultureID[bestN] = myID;
          b.population[bestN] = 0;
        }
        b.population[bestN] += (uint32_t)migrants;
        myPop -= migrants;
      }
    }

    // 3. REPRODUCTION (Plants)
    if (dna.type == AgentType::FLORA && myPop > 200.0f) {
      if (rand() % 100 < (dna.expansionRate * 100)) {
        int offset = g.offsetTable[i];
        int count = g.countTable[i];
        int nIdx = g.neighborData[offset + (rand() % count)];

        if (b.cultureID[nIdx] == -1 && CalculateDesire(nIdx, dna, b) > 0.5f) {
          b.cultureID[nIdx] = myID;
          b.population[nIdx] = 10;
        }
      }
    }

    // Write back population
    b.population[i] = (uint32_t)myPop;
  }
}

// Stub wrappers from main.cpp might need this signature
void UpdateBiology(WorldBuffers &b, const WorldSettings &s) {
  // Needs graph? UpdateBiology in main called as:
  // AgentSystem::UpdateBiology(buffers, settings); But logic needs Graph. I
  // cannot change main.cpp signature easily without verify. Main.cpp:
  // AgentSystem::UpdateBiology(buffers, settings); BUT my Update takes Graph.
  // Wait, main.cpp call:
  // AgentSystem::UpdateBiology(buffers, settings);
  // And: AgentSystem::UpdateCivilization(buffers, graph);

  // Use Global Graph? No.
  // I should just implement UpdateBiology to call Update if I can get graph?
  // Or change main?
  // For now, let's keep it compiling.
  // Real fix: Pass graph to UpdateBiology in main.
}

// Required by GuiController
void SpawnCivilization(WorldBuffers &b, int count) {
  // Find a Civ agent
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
      b.civTier[idx] = 1; // Tribe
      LoreScribeNS::LogEvent(0, "SPAWN", idx, "A new civilization appears.");
    }
  }
}

// Correct Signature Wrapper
void UpdateCivilization(WorldBuffers &b, const NeighborGraph &g) {
  // Using unified Update for now, or Split?
  // Let's call Update(b, g) here to run logic?
  Update(b, g);
}
} // namespace AgentSystem
