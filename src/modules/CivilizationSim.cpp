#include "../../include/AssetManager.hpp"
#include "../../include/SimulationModules.hpp"
#include <algorithm>
#include <cmath>


namespace CivilizationSim {

// Tier thresholds
const float TRIBE_POP = 100.0f;
const float VILLAGE_POP = 500.0f;
const float VILLAGE_FOOD = 200.0f;
const float TOWN_POP = 2000.0f;
const float TOWN_STONE = 100.0f;
const float CITY_POP = 10000.0f;

void EvolveSettlements(WorldBuffers &b) {
  if (!b.civTier || !b.population)
    return;

  for (uint32_t i = 0; i < b.count; ++i) {
    float pop = (float)b.population[i];
    int currentTier = b.civTier[i];

    // Get resources (Food=0, Wood=1, Stone=2)
    float food = b.GetResource(i, 0);
    float stone = b.GetResource(i, 2);

    // Tier evolution logic
    int newTier = 0; // Wild

    if (pop >= CITY_POP && stone >= TOWN_STONE * 3) {
      newTier = 4; // City
    } else if (pop >= TOWN_POP && stone >= TOWN_STONE) {
      newTier = 3; // Town
    } else if (pop >= VILLAGE_POP && food >= VILLAGE_FOOD) {
      newTier = 2; // Village
    } else if (pop >= TRIBE_POP) {
      newTier = 1; // Tribe
    }

    // Only allow tier increase (no sudden collapse)
    if (newTier > currentTier) {
      b.civTier[i] = newTier;

      // Log major upgrades
      if (newTier >= 3) {
        LoreScribeNS::LogEvent(0, "UPGRADE", i,
                               "Settlement grew to tier " +
                                   std::to_string(newTier));
      }
    }
  }
}

void ConstructBuildings(WorldBuffers &b) {
  if (!b.buildingID || !b.civTier)
    return;

  for (uint32_t i = 0; i < b.count; ++i) {
    int tier = b.civTier[i];
    int currentBuilding = b.buildingID[i];

    // Only build if we have enough resources and tier
    if (tier < 1 || currentBuilding > 0)
      continue; // Need a tribe, already has building

    float wood = b.GetResource(i, 1);
    float stone = b.GetResource(i, 2);
    float food = b.GetResource(i, 0);

    // Building priorities based on needs
    if (tier >= 2 && stone >= 50.0f && wood >= 50.0f) {
      // Stronghold (ID 3) - provides defense
      b.buildingID[i] = 3;
      b.AddResource(i, 1, -50.0f); // Wood cost
      b.AddResource(i, 2, -50.0f); // Stone cost
      if (b.defense)
        b.defense[i] += 20.0f;
    } else if (tier >= 1 && wood >= 30.0f && food >= 20.0f) {
      // Logging Camp (ID 1) - produces wood
      b.buildingID[i] = 1;
      b.AddResource(i, 0, -20.0f); // Food cost
    } else if (tier >= 2 && food >= 50.0f && wood >= 20.0f) {
      // Stone Quarry (ID 2) - produces stone
      b.buildingID[i] = 2;
      b.AddResource(i, 0, -30.0f);
      b.AddResource(i, 1, -20.0f);
    }
  }
}

void ProduceResources(WorldBuffers &b) {
  if (!b.buildingID)
    return;

  for (uint32_t i = 0; i < b.count; ++i) {
    int building = b.buildingID[i];

    // Production based on building type
    switch (building) {
    case 1:                      // Logging Camp
      b.AddResource(i, 1, 2.0f); // +2 Wood
      break;
    case 2:                      // Stone Quarry
      b.AddResource(i, 2, 2.0f); // +2 Stone
      break;
    case 3: // Stronghold - no production but provides defense
      break;
    case 4:                      // Farm
      b.AddResource(i, 0, 3.0f); // +3 Food
      break;
    }

    // Natural food production based on biome
    if (b.moisture && b.temperature) {
      float m = b.moisture[i];
      float t = b.temperature[i];

      // Fertile land produces food naturally
      if (m > 0.4f && m < 0.8f && t > 0.3f && t < 0.7f) {
        b.AddResource(i, 0, 0.5f); // Natural food
      }
    }
  }
}

void Update(WorldBuffers &b, const NeighborGraph &g, const WorldSettings &s) {
  // Run all civilization sub-systems
  EvolveSettlements(b);
  ConstructBuildings(b);
  ProduceResources(b);
}

} // namespace CivilizationSim
