#include "../../include/AssetManager.hpp"
#include "../../include/Lore.hpp"
#include "../../include/Simulation.hpp"
#include <iostream>

namespace CivilizationSim {

void Update(WorldBuffers &b, const NeighborGraph &g, const WorldSettings &s) {
  if (!b.cultureID)
    return;

  const float DEATH_RATE_OLD_AGE = 0.995f;
  const float ACCIDENT_RATE = 0.001f;

  for (uint32_t i = 0; i < b.count; ++i) {
    int id = b.cultureID[i];
    if (id == -1 || id >= (int)AssetManager::agentRegistry.size())
      continue;

    const AgentDefinition &def = AssetManager::agentRegistry[id];
    float pop = (float)b.population[i];

    // Natural Causes
    pop *= DEATH_RATE_OLD_AGE;
    if ((rand() % 10000) / 10000.0f < ACCIDENT_RATE) {
      pop *= 0.8f;
    }

    // Construction
    if (def.type == AgentType::CIVILIZED && b.structureType) {
      float wood = b.GetResource(i, 1);
      float stone = b.GetResource(i, 2); // Assuming Stone is ID 2
      uint8_t &structure = b.structureType[i];

      if (structure == 0 && pop > 100.0f && wood > 50.0f) {
        structure = 1; // Road
        b.AddResource(i, 1, -50.0f);
      } else if (structure == 1 && pop > 500.0f && wood > 100.0f &&
                 stone > 20.0f) {
        structure = 2; // Village
        b.AddResource(i, 1, -100.0f);
        b.AddResource(i, 2, -20.0f);
        LoreScribeNS::LogEvent(0, "FOUNDING", i, "A new village established.");
      } else if (structure == 2 && pop > 2000.0f && stone > 500.0f) {
        structure = 3; // City
        b.AddResource(i, 2, -500.0f);
        LoreScribeNS::LogEvent(0, "CITY", i, "A village grew into a city.");
      }
    }
    b.population[i] = (uint32_t)pop;
  }
}
} // namespace CivilizationSim
