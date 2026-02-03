#include "../../include/SimulationModules.hpp"
#include <cmath>
#include <vector>

void AgentSystem::TickAgents(WorldBuffers& b, float deltaTime, uint32_t count) {

    // Loop through all cells to update populations
    for (uint32_t i = 0; i < count; ++i) {
        // Skip empty cells to save CPU cycles
        if (b.population[i] <= 0) continue;

        // 1. ENVIRONMENTAL STRESS (Evolution)
        // If the cell is too hot/cold compared to the agent's preference (stored in a map or simplified here)
        // For this snippet, we assume a "Standard Human" preference of 0.5 Temp
        float idealTemp = 0.5f;
        float stress = std::abs(b.temperature[i] - idealTemp);

        if (stress > 0.2f) {
            // High stress reduces population
            b.population[i] -= (uint32_t)(stress * 10.0f * deltaTime);
        } else {
            // Comfort increases population (Logistic Growth)
            if (b.population[i] < 1000) { // Carrying capacity
                b.population[i] += 1;
            }
        }

        // 2. INFRASTRUCTURE BUILDING
        // If population is high enough, they build "Roads" or "Cities"
        if (b.population[i] > 500) {
            // Increase infrastructure score (Civilization visualization)
            b.infrastructure[i] += 0.01f * deltaTime;
            if (b.infrastructure[i] > 1.0f) b.infrastructure[i] = 1.0f;
        }
    }
}
