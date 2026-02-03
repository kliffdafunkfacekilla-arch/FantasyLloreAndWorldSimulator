#include "../../include/SimulationModules.hpp"
#include <iostream>

void ChronosSystem::GlobalChronosTick(WorldBuffers& buffers, ChronosConfig& time, float realDeltaTime) {
    // 1. Advance the clock
    time.currentDayProgress += realDeltaTime / time.dayLengthInSeconds;

    if (time.currentDayProgress >= 1.0f) {
        // --- DAILY LOOP ---
        UpdateWeatherSystems(buffers);
        ProcessAgentMovement(buffers);
        
        time.currentDayProgress = 0;
        time.dayCount++;
        std::cout << "[CHRONOS] Day " << time.dayCount << " ended.\n";
    }

    if (time.dayCount >= time.daysPerMonth) {
        // --- MONTHLY LOOP ---
        RegenerateResources(buffers); // Forests grow, Fish spawn
        ProcessRandomEvents();       // Roll for Boons/Disasters
        
        time.dayCount = 0;
        time.monthCount++;
        std::cout << "[CHRONOS] Month " << time.monthCount << " started.\n";
    }
}

void ChronosSystem::UpdateWeatherSystems(WorldBuffers& buffers) {
    // Call ClimateSim if available
    if (climate && settings) {
        // Recalculate dynamic weather (simplification: re-run global climate)
        climate->UpdateGlobalClimate(buffers, *settings);
        // Could vary wind strength here
    }
}

void ChronosSystem::ProcessAgentMovement(WorldBuffers& buffers) {
    // Call AgentSystem if available
    if (agents && registry && settings) {
        // Delta time for movement is 1 day (conceptual)
        agents->TickAgents(buffers, *registry, 1.0f, settings->cellCount);
    }
}

void ChronosSystem::RegenerateResources(WorldBuffers& buffers) {
    if (buffers.resourceLevels) {
        // Example: Grow resource 0 (Food/Wood) by 0.1
        // Assuming settings->cellCount valid
        if(!settings) return;
        
        // This loop matches "resourceCount" implicitly if we knew it.
        // Simplified: just assuming resource 0 is renewable
        for(uint32_t i=0; i<settings->cellCount; ++i) {
             // Index 0 stride
             if (settings->resourceCount > 0)
                buffers.resourceLevels[i * settings->resourceCount] += 0.5f; 
        }
    }
}

void ChronosSystem::ProcessRandomEvents() {
    // Placeholder
    // Rolling a dice...
    // std::cout << "Event: A mild breeze passes.\n";
}
