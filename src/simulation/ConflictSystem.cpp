#include "../../include/AssetManager.hpp"
#include "../../include/Lore.hpp"
#include "../../include/Simulation.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace ConflictSystem {

void Update(WorldBuffers &b, const NeighborGraph &g, const WorldSettings &s) {
  if (!b.cultureID)
    return;

  float banditThreshold = 0.05f;
  float battleDamage = 0.1f;

  for (uint32_t i = 0; i < b.count; ++i) {
    int myID = b.cultureID[i];
    if (myID == -1 || myID >= (int)AssetManager::agentRegistry.size())
      continue;

    const AgentDefinition &myDef = AssetManager::agentRegistry[myID];
    // Cast population to float for checks
    float myStr = (float)b.population[i];

    // 1. CRIME
    if (myDef.type == AgentType::CIVILIZED && b.structureType) {
      float wealth = (b.GetResource(i, 1) + b.GetResource(i, 2));
      float security = (b.structureType[i] * 10.0f) + (myStr * 0.01f);

      if (wealth > 100.0f && security < 20.0f) {
        if ((rand() % 1000) / 1000.0f < banditThreshold) {
          float stolen = wealth * 0.2f;
          b.AddResource(i, 1, -stolen / 2.0f);
          myStr *= 0.9f;
          LoreScribeNS::LogEvent(0, "CRIME", i, "Bandits raided a settlement.");
        }
      }
    }

    // 2. WAR
    int offset = g.offsetTable[i];
    int count = g.countTable[i];

    for (int k = 0; k < count; ++k) {
      int nIdx = g.neighborData[offset + k];
      int theirID = b.cultureID[nIdx];

      if (theirID == -1 || theirID == myID ||
          theirID >= (int)AssetManager::agentRegistry.size())
        continue;

      const AgentDefinition &theirDef = AssetManager::agentRegistry[theirID];
      float theirStr = (float)b.population[nIdx];

      bool isWar = false;
      if (myDef.type == AgentType::FAUNA && myDef.aggression > 0.5f)
        isWar = true;
      if (myDef.type == AgentType::CIVILIZED &&
          theirDef.type == AgentType::CIVILIZED) {
        if (myDef.aggression > 0.3f)
          isWar = true;
      }

      if (isWar) {
        float damage = myStr * battleDamage * myDef.aggression;
        float defense = 0.0f;
        if (b.structureType)
          defense = (b.structureType[nIdx] * 5.0f);

        float actualDamage = std::max(0.0f, damage - defense);
        theirStr -= actualDamage;

        if (actualDamage > 0 && myDef.type == AgentType::FAUNA) {
          myStr += actualDamage * 0.5f;
        }

        if (theirStr <= 0.0f) {
          b.cultureID[nIdx] = myID;
          b.population[nIdx] = (uint32_t)(myStr * 0.2f);
          myStr *= 0.8f;
          if (myDef.type == AgentType::CIVILIZED) {
            LoreScribeNS::LogEvent(0, "CONQUEST", nIdx,
                                   myDef.name + " conquered territory.");
          }
        } else {
          b.population[nIdx] = (uint32_t)theirStr;
        }
      }
    }
    b.population[i] = (uint32_t)myStr;
  }
}
} // namespace ConflictSystem
