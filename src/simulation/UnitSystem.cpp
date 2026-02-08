#include "../../include/AssetManager.hpp"
#include "../../include/SimulationModules.hpp"
#include <algorithm>
#include <cmath>

namespace UnitSystem {

void MoveUnits(WorldBuffers &b, int mapWidth) {
  for (auto &u : AssetManager::activeUnits) {
    if (!u.isAlive)
      continue;

    // Calculate direction to target
    float dx = (float)u.targetX - u.x;
    float dy = (float)u.targetY - u.y;
    float dist = std::sqrt(dx * dx + dy * dy);

    if (dist < 1.0f) {
      // Arrived at destination
      continue;
    }

    // Normalize and apply speed
    float nx = dx / dist;
    float ny = dy / dist;

    u.x += nx * u.speed;
    u.y += ny * u.speed;

    // Terrain speed modifier (skip for airships)
    if (u.type != UnitType::AIRSHIP) {
      int cellIdx = (int)u.y * mapWidth + (int)u.x;
      if (cellIdx >= 0 && cellIdx < (int)b.count) {
        float h = b.height[cellIdx];
        if (h > 0.7f)
          u.x -= nx * u.speed * 0.5f; // Mountains slow by 50%
      }
    }
  }
}

void DeliverCargo(WorldBuffers &b, int mapWidth) {
  for (auto &u : AssetManager::activeUnits) {
    if (!u.isAlive || u.type != UnitType::TRADER)
      continue;

    float dx = (float)u.targetX - u.x;
    float dy = (float)u.targetY - u.y;
    float dist = std::sqrt(dx * dx + dy * dy);

    if (dist < 1.5f && u.resourceAmount > 0) {
      // Deliver at destination
      int cellIdx = u.targetY * mapWidth + u.targetX;
      if (cellIdx >= 0 && cellIdx < (int)b.count) {
        b.AddResource(cellIdx, u.resourceID, u.resourceAmount);
        u.resourceAmount = 0.0f;
        u.isAlive = false; // Unit disbands after delivery
      }
    }
  }
}

void ResolveCombat(WorldBuffers &b, int mapWidth) {
  for (auto &u : AssetManager::activeUnits) {
    if (!u.isAlive || u.type != UnitType::ARMY)
      continue;

    float dx = (float)u.targetX - u.x;
    float dy = (float)u.targetY - u.y;
    float dist = std::sqrt(dx * dx + dy * dy);

    if (dist < 1.5f) {
      // At destination - attempt conquest
      int cellIdx = u.targetY * mapWidth + u.targetX;
      if (cellIdx < 0 || cellIdx >= (int)b.count)
        continue;

      int targetFaction = b.factionID[cellIdx];

      // Check diplomacy
      float relation = AssetManager::GetRelation(u.factionID, targetFaction);
      if (relation > 50.0f)
        continue; // Allies - don't attack

      float defense = (b.defense) ? b.defense[cellIdx] : 0.0f;
      float pop = (float)b.population[cellIdx];
      float defenseScore = pop + defense * 10.0f;

      if (u.combatStrength > defenseScore * 0.8f) {
        // Victory - capture cell
        b.factionID[cellIdx] = u.factionID;
        b.population[cellIdx] = (uint32_t)(b.population[cellIdx] * 0.5f);

        if (b.chaos)
          b.chaos[cellIdx] += 0.1f;

        LoreScribeNS::LogEvent(0, "ARMY_VICTORY", cellIdx,
                               "Military conquest by faction " +
                                   std::to_string(u.factionID));

        u.combatStrength *= 0.5f; // Army weakened
        if (u.combatStrength < 5.0f)
          u.isAlive = false;
      } else {
        // Defeat
        u.isAlive = false;
      }
    }
  }
}

void CleanupDeadUnits() {
  auto &units = AssetManager::activeUnits;
  units.erase(std::remove_if(units.begin(), units.end(),
                             [](const Unit &u) { return !u.isAlive; }),
              units.end());
}

void Update(WorldBuffers &b, int mapWidth) {
  MoveUnits(b, mapWidth);
  DeliverCargo(b, mapWidth);
  ResolveCombat(b, mapWidth);

  // Cleanup dead units every 10 ticks (using static counter)
  static int cleanupCounter = 0;
  if (++cleanupCounter >= 10) {
    CleanupDeadUnits();
    cleanupCounter = 0;
  }
}

} // namespace UnitSystem
