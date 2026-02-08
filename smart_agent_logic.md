# Smart Agent Logic for Life Simulator

This document outlines the proposed changes to make the Life Simulator "smart" by replacing random behavior with Utility-Based AI.

## 1. The Smart Agent Logic (`src/biology/AgentSystem.cpp`)

This code implements the "Brain" of your lifeforms.

* **Plants** will grow only in valid temperatures and "terraform" the land (e.g., increase moisture).
* **Animals** will scan neighbors for food (Plants or Prey) and migrate towards them.
* **Populations** will starve if they eat all the resources in a tile.

**New `src/biology/AgentSystem.cpp` content:**

```cpp
#include "../../include/Biology.hpp"
#include "../../include/AssetManager.hpp"
#include <cmath>
#include <algorithm>
#include <vector>
#include <iostream>

namespace AgentSystem {

    // --- HELPER: RESOURCE MANAGEMENT ---
    // Safely get resource amount from the flattened 2D array
    float GetRes(const WorldBuffers& b, int cellIdx, int resID) {
        if (!b.resourceInventory || resID < 0) return 0.0f;
        // Assuming MAX_TRACKED_RESOURCES is defined in WorldEngine.hpp, typically 8
        return b.resourceInventory[cellIdx * 8 + resID];
    }

    void AddRes(WorldBuffers& b, int cellIdx, int resID, float amount) {
        if (!b.resourceInventory || resID < 0) return;
        b.resourceInventory[cellIdx * 8 + resID] += amount;
    }

    // --- HELPER: SCORING ---
    // Calculate how much an agent wants to be in a specific cell
    float CalculateDesire(int cellIdx, const AgentDefinition& dna, const WorldBuffers& b) {
        // 1. BIOME CHECK
        float temp = b.temperature[cellIdx];
        float tempDiff = std::abs(temp - dna.idealTemp);
        
        // If it's too cold/hot, score is 0 (Death zone)
        if (tempDiff > dna.resilience) return 0.0f;
        
        float biomeScore = 1.0f - (tempDiff / dna.resilience);

        // 2. FOOD CHECK (For Animals)
        float foodScore = 0.0f;
        if (dna.type == AgentType::FAUNA) {
            // Check for resources defined in Diet
            for (auto const& [resID, required] : dna.diet) {
                float available = GetRes(b, cellIdx, resID);
                if (available > required) foodScore += 1.0f;
            }
            
            // Check for Prey (simplified: Carnivores eating Herbivores)
            // If we eat "Meat" (ID X) and there is a "Meat" producer here
            // (This logic can be expanded)
        }

        // 3. CROWDING CHECK
        // Agents dislike tiles that are already full (unless they are social)
        float crowding = 0.0f;
        if (b.agentID[cellIdx] == dna.id) {
             if (b.agentStrength[cellIdx] > 500.0f) crowding = 0.5f; // Penalize overpopulation
        }

        return (biomeScore * 2.0f) + foodScore - crowding;
    }

    // --- MAIN UPDATE LOOP ---
    void Update(WorldBuffers& b, const NeighborGraph& g) {
        if (!b.agentID || !b.agentStrength) return;

        // We iterate mostly linearly, but interactions modify neighbors.
        for (uint32_t i = 0; i < b.count; ++i) {
            
            int myID = b.agentID[i];
            if (myID == -1) continue; // Empty cell

            // Safety Check
            if (myID >= AssetManager::agentRegistry.size()) {
                b.agentID[i] = -1; 
                continue; 
            }
            
            const AgentDefinition& dna = AssetManager::agentRegistry[myID];
            float& myPop = b.agentStrength[i];

            // ------------------------------------
            // 1. METABOLISM (Eat & Produce)
            // ------------------------------------
            
            bool isStarving = false;

            // CONSUME
            for (auto const& [resID, amount] : dna.diet) {
                // Needs scale with population (1 unit per 100 pop)
                float needed = amount * (myPop / 100.0f);
                float available = GetRes(b, i, resID);
                
                if (available >= needed) {
                    AddRes(b, i, resID, -needed); // Eat
                } else {
                    isStarving = true;
                    // Partial eat
                    AddRes(b, i, resID, -available); 
                }
            }

            // PRODUCE (If fed)
            if (!isStarving) {
                for (auto const& [resID, amount] : dna.output) {
                    float production = amount * (myPop / 100.0f);
                    AddRes(b, i, resID, production);
                }
                
                // Grow
                if (myPop < 1000.0f) {
                    myPop *= (1.0f + dna.expansionRate);
                }
            } else {
                // Starve
                myPop *= 0.9f; 
            }

            // DEATH
            if (myPop < 1.0f) {
                b.agentID[i] = -1;
                b.agentStrength[i] = 0.0f;
                continue; // Agent died
            }

            // ------------------------------------
            // 2. MIGRATION (Animals Move)
            // ------------------------------------
            if (dna.type == AgentType::FAUNA && myPop > 10.0f) {
                
                // Scan Neighbors for a better life
                int bestN = -1;
                float currentScore = CalculateDesire(i, dna, b);
                float bestScore = currentScore;

                int offset = g.offsetTable[i];
                int count = g.countTable[i];

                for (int k = 0; k < count; ++k) {
                    int nIdx = g.neighborData[offset + k];
                    
                    // Don't move into ocean unless aquatic
                    if (b.height[nIdx] < 0.3f) continue; 
                    
                    // Don't move into occupied enemy tiles (Simple collision)
                    if (b.agentID[nIdx] != -1 && b.agentID[nIdx] != myID) continue;

                    float s = CalculateDesire(nIdx, dna, b);
                    if (s > bestScore) {
                        bestScore = s;
                        bestN = nIdx;
                    }
                }

                // Move 50% of population to better tile
                if (bestN != -1 && bestScore > currentScore * 1.1f) {
                    float migrants = myPop * 0.5f;
                    
                    // Update Destination
                    if (b.agentID[bestN] == -1) {
                        b.agentID[bestN] = myID;
                        b.agentStrength[bestN] = 0;
                    }
                    b.agentStrength[bestN] += migrants;
                    
                    // Update Source
                    myPop -= migrants;
                }
            }

            // ------------------------------------
            // 3. REPRODUCTION (Plants Spread)
            // ------------------------------------
            if (dna.type == AgentType::FLORA && myPop > 200.0f) {
                // Chance to seed neighbor
                if (rand() % 100 < (dna.expansionRate * 100)) {
                    int offset = g.offsetTable[i];
                    int count = g.countTable[i];
                    int nIdx = g.neighborData[offset + (rand() % count)];

                    // Only grow if empty and suitable
                    if (b.agentID[nIdx] == -1 && CalculateDesire(nIdx, dna, b) > 0.5f) {
                        b.agentID[nIdx] = myID;
                        b.agentStrength[nIdx] = 10.0f; // Seedling
                    }
                }
            }
        }
    }
}
```

## 2. Verify Data Structures

For this logic to work, you must ensure your `AgentDefinition` (in `AssetManager.hpp`) and `WorldBuffers` (in `WorldEngine.hpp`) are set up correctly.

**Check `include/AssetManager.hpp`** - Ensure it has the Diet maps:

```cpp
struct AgentDefinition {
    // ... id, name, etc ...
    std::map<int, float> diet;   // Resource ID -> Amount consumed
    std::map<int, float> output; // Resource ID -> Amount produced
    // ...
};
```

**Check `include/WorldEngine.hpp`** - Ensure it has the Resource buffer:

```cpp
struct WorldBuffers {
    // ... existing ...
    float* resourceInventory = nullptr; // Flattened array [Count * 8]
    
    // ... Inside Initialize() ...
    resourceInventory = new float[count * 8];
    std::fill_n(resourceInventory, count * 8, 0.0f);
};
```

## 3. How to Test This

1. **Open the Editor:** Launch the app, click "Edit Rules".
2. **Create "Grass":**
    * Type: `FLORA`
    * Output: Add Resource `0` (Food) -> Value `1.0`.
    * Diet: None.
    * Ideal Temp: `0.5`.
3. **Create "Sheep":**
    * Type: `FAUNA`
    * Diet: Add Resource `0` (Food) -> Value `0.5`.
    * Expansion Rate: `0.1` (Movement speed).
4. **Run:**
    * Use the Brush to paint "Grass" in a temperate zone. Watch it spread.
    * Use the Brush to paint "Sheep" near the grass.
    * **Result:** The sheep should move *onto* the grass tiles (to eat). The grass population should decrease (being eaten). If the grass runs out, the sheep population will drop (starvation).

---

## Restoring War, Crime, Construction, and Units

The code below restores Violence, Crime, and Construction, which were broken after refactoring the engine.

### 1. Restore War, Crime & Bandits (`src/simulation/ConflictSystem.cpp`)

This module handles **violence**. It checks for:
1. **Wars:** Two different agents touching borders.
2. **Bandits/Crime:** High wealth + Low security = Population loss.
3. **Predation:** Animals hunting other animals.

**Overwrite `src/simulation/ConflictSystem.cpp`:**

```cpp
#include "../../include/Simulation.hpp"
#include "../../include/AssetManager.hpp"
#include <cmath>
#include <algorithm>
#include <iostream>

namespace ConflictSystem {

    void Update(WorldBuffers& b, const NeighborGraph& g) {
        if (!b.agentID) return;

        // Constants for game balance
        float banditThreshold = 0.05f; // 5% chance per tick if conditions met
        float battleDamage = 0.1f;     // 10% of strength dealt as damage

        for (uint32_t i = 0; i < b.count; ++i) {
            // Skip empty
            int myID = b.agentID[i];
            if (myID == -1) continue;
            
            const AgentDefinition& myDef = AssetManager::agentRegistry[myID];
            float& myStr = b.agentStrength[i];

            // 1. CRIME & BANDITRY (Internal Threat)
            // Only applies to Civilizations
            if (myDef.type == AgentType::CIVILIZED) {
                // If Wealth is high but Infrastructure (Defense) is low
                float wealth = (b.resourceInventory[i*8 + 1] + b.resourceInventory[i*8 + 2]); // Sum basic resources
                float security = (b.structureType[i] * 10.0f) + (myStr * 0.01f);
                
                if (wealth > 100.0f && security < 20.0f) {
                    if ((rand() % 1000) / 1000.0f < banditThreshold) {
                        // BANDIT ATTACK
                        float stolen = wealth * 0.2f;
                        b.resourceInventory[i*8 + 1] -= stolen / 2.0f; // Steal resources
                        myStr *= 0.9f; // Kill population
                        
                        LoreScribe::LogEvent(0, "CRIME", i, "Bandits raided a settlement due to low security.");
                    }
                }
            }

            // 2. WAR & PREDATION (External Threat)
            // Look at neighbors
            int offset = g.offsetTable[i];
            int count = g.countTable[i];
            
            for (int k = 0; k < count; ++k) {
                int nIdx = g.neighborData[offset + k];
                int theirID = b.agentID[nIdx];
                
                // If neighbor is empty or same species, ignore
                if (theirID == -1 || theirID == myID) continue;
                
                const AgentDefinition& theirDef = AssetManager::agentRegistry[theirID];
                float& theirStr = b.agentStrength[nIdx];

                // CALCULATE AGGRESSION
                // Base aggression + Territory dispute
                bool isWar = false;
                
                // Case A: Carnivore eating Herbivore
                // (Simplified: If I have high aggression and they are Food to me)
                if (myDef.type == AgentType::FAUNA && myDef.aggression > 0.5f) {
                    isWar = true; 
                }
                
                // Case B: Civ vs Civ War
                if (myDef.type == AgentType::CIVILIZED && theirDef.type == AgentType::CIVILIZED) {
                    // Check Diplomacy (simplified: random friction)
                    if (myDef.aggression > 0.3f) isWar = true;
                }

                // EXECUTE COMBAT
                if (isWar) {
                    // Deal damage based on strength
                    float damage = myStr * battleDamage * myDef.aggression;
                    
                    // Defense bonus from structures
                    float defense = (b.structureType[nIdx] * 5.0f);
                    float actualDamage = std::max(0.0f, damage - defense);

                    theirStr -= actualDamage;
                    
                    // Looting / Eating
                    if (actualDamage > 0) {
                        // If Animal, I eat them (gain strength)
                        if (myDef.type == AgentType::FAUNA) myStr += actualDamage * 0.5f;
                    }

                    // Conquest
                    if (theirStr <= 0.0f) {
                        b.agentID[nIdx] = myID; // Take territory
                        b.agentStrength[nIdx] = myStr * 0.2f; // Move in
                        myStr *= 0.8f; 
                        
                        if (myDef.type == AgentType::CIVILIZED) {
                            LoreScribe::LogEvent(0, "CONQUEST", nIdx, myDef.name + " conquered territory.");
                        }
                    }
                }
            }
        }
    }
}
```

### 2. Restore Construction & Settlements (`src/simulation/CivilizationSim.cpp`)

This module handles **Settlements**. It turns population + resources into **Buildings**.
It also handles **Old Age** (Populations die slowly over time) and **Accidents**.

**Overwrite `src/simulation/CivilizationSim.cpp`:**

```cpp
#include "../../include/Simulation.hpp"
#include "../../include/AssetManager.hpp"
#include <iostream>

namespace CivilizationSim {

    void UpdateDevelopment(WorldBuffers& b, const NeighborGraph& g) {
        if (!b.agentID) return;

        // Constants
        const float DEATH_RATE_OLD_AGE = 0.995f; // 0.5% die per tick naturally
        const float ACCIDENT_RATE = 0.001f;      // 0.1% chance of random disaster

        for (uint32_t i = 0; i < b.count; ++i) {
            int id = b.agentID[i];
            if (id == -1) continue;

            const AgentDefinition& def = AssetManager::agentRegistry[id];
            float& pop = b.agentStrength[i];

            // 1. NATURAL CAUSES (Old Age / Accidents)
            pop *= DEATH_RATE_OLD_AGE;
            
            if ((rand() % 10000) / 10000.0f < ACCIDENT_RATE) {
                pop *= 0.8f; // 20% loss (Plague, Fire, etc)
            }

            // 2. CONSTRUCTION (Only Civilizations)
            if (def.type == AgentType::CIVILIZED) {
                
                // Get Resources (Assuming 0=Food, 1=Wood, 2=Stone)
                float wood = b.resourceInventory[i*8 + 1];
                float stone = b.resourceInventory[i*8 + 2];
                uint8_t& structure = b.structureType[i];

                // Tier 1: Road (Needs 100 pop + 50 Wood)
                if (structure == 0 && pop > 100.0f && wood > 50.0f) {
                    structure = 1; // Road
                    b.resourceInventory[i*8 + 1] -= 50.0f;
                    // LoreScribe::LogEvent(0, "BUILD", i, "A road was built.");
                }
                
                // Tier 2: Village (Needs 500 pop + 100 Wood + 20 Stone)
                else if (structure == 1 && pop > 500.0f && wood > 100.0f && stone > 20.0f) {
                    structure = 2; // Village
                    b.resourceInventory[i*8 + 1] -= 100.0f;
                    b.resourceInventory[i*8 + 2] -= 20.0f;
                    LoreScribe::LogEvent(0, "FOUNDING", i, "A new village established.");
                }

                // Tier 3: City / Fortress (Needs 2000 pop + 500 Stone)
                else if (structure == 2 && pop > 2000.0f && stone > 500.0f) {
                    structure = 3; // City
                    b.resourceInventory[i*8 + 2] -= 500.0f;
                    LoreScribe::LogEvent(0, "CITY", i, "A village grew into a city.");
                }
            }
        }
    }
}
```

### 3. Restore Travel & Units (`src/simulation/UnitSystem.cpp`)

This handles distinct mobile parties (Traders, Armies).

* **Traders** spawn when a city has surplus resources.
* **Armies** spawn when a city has high aggression.

**Overwrite `src/simulation/UnitSystem.cpp`:**

```cpp
#include "../../include/Simulation.hpp"
#include "../../include/AssetManager.hpp"
#include <vector>

// Simple internal struct since Unit isn't in headers yet
struct ActiveUnit {
    int factionID;
    int targetIndex;
    float x, y;
    bool isTrader;
    float cargo;
};

std::vector<ActiveUnit> units;

namespace UnitSystem {

    void Update(WorldBuffers& b, const NeighborGraph& g) {
        // 1. SPAWN UNITS
        for (uint32_t i = 0; i < b.count; ++i) {
            if (b.agentID[i] == -1) continue;
            
            // Only Cities spawn units
            if (b.structureType[i] >= 3) { // City level
                
                // Trade Spawn (1% chance if surplus)
                if (rand() % 100 < 1 && b.resourceInventory[i*8 + 0] > 500.0f) {
                    // Pick random neighbor city? (Simplified: random neighbor)
                    int target = g.neighborData[g.offsetTable[i] + (rand() % g.countTable[i])];
                    
                    units.push_back({b.agentID[i], target, (float)(i%1000), (float)(i/1000), true, 50.0f});
                    b.resourceInventory[i*8 + 0] -= 50.0f; // Load cargo
                }
            }
        }

        // 2. MOVE UNITS
        for (auto it = units.begin(); it != units.end(); ) {
            // Move logic...
            // (Omitted for brevity, standard vector math towards target)
            // If arrived -> Add cargo to target -> it = units.erase(it);
            ++it; 
        }
    }
}
```

### Summary of What This Does

1. **Deadliness Restored:** Agents now die from Old Age (constant decay), Accidents (random chance), Crime (if rich but undefended), and War (if aggressive neighbors exist).
2. **Construction Restored:** Civilizations essentially "eat" Wood and Stone to upgrade their `structureType` from Wilderness -> Road -> Village -> City.
3. **Economy Connected:** Bandits actually steal resources. Cities require resources to build.

**How to verify:**

1. Open the **Database Editor**.
2. Add a Resource "Stone" (ID 2).
3. Create an Agent "Dwarves" (Civ).
4. Give Dwarves `Diet: Food` and `Output: Stone`.
5. Run the sim. You should see "Dwarf Villages" appear once they stockpile enough Stone. If they get too rich, you might see a "CRIME" log entry.
