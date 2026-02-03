#include "../../include/SimulationModules.hpp"
#include "../../include/WorldEngine.hpp"
#include <vector>

void SimulationLoop::Update(WorldBuffers &buffers,
                            std::vector<AgentTemplate> &registry,
                            std::vector<Faction> &factions, ChronosConfig &time,
                            const NeighborGraph &graph, float dt) {

  // 1. Physical Layer
  ClimateSim climate;
  climate.UpdateGlobalClimate(buffers, settings);

  // 2. Life Layer
  agentSystem.TickAgents(buffers, registry, dt, buffers.count);

  // 3. Logistics Layer
  if (time.dayCount % 7 == 0) {
    for (int rID = 0; rID < (int)settings.resourceCount; ++rID) {
      ProcessLogistics(buffers, factions, buffers.count, rID);
    }
  }

  // 4. Conflict Layer (Lore integrated)
  if (time.dayCount % 2 == 0) {
    conflictSystem.ResolveBorders(buffers, registry, factions, graph, scribe,
                                  time, dt);
  }

  // 5. Timelapse Snapshot (Monthly)
  // Assuming 30 days per month
  // If dayCount wraps, we need to check cycle.
  // In ChronosSystem logic, dayCount resets. So if dayCount == 0, it's new
  // month.
  if (time.dayCount == 0) {
    timelapse.TakeSnapshot(buffers, time);
  }
}
