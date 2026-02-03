#include "../../include/SimulationModules.hpp"
#include <cmath>
#include <iostream>

void AgentSystem::TickAgents(WorldBuffers& b, std::vector<AgentTemplate>& registry, float deltaTime, uint32_t count) {
    if (!b.population || !b.speciesID) return;

    for (uint32_t i = 0; i < b.count; ++i) { // Use b.count from buffer
        if (b.population[i] <= 0) continue;

        int speciesIdx = b.speciesID[i];
        if (speciesIdx < 0 || speciesIdx >= (int)registry.size()) continue;

        AgentTemplate& dna = registry[speciesIdx];

        // 1. BIOME PRESSURE & EVOLUTION
        // Use b.temperature (WorldEngine.hpp uses 'temperature' not 'temp')
        if (b.temperature) {
            float tempDiff = b.temperature[i] - dna.idealTemp;
            
            // If the cell is too hot/cold, the species "drifts" to adapt
            if (std::abs(tempDiff) > 0.1f) {
                dna.idealTemp += tempDiff * dna.adaptiveRate * deltaTime;
            }
        }

        // 2. DOMESTICATION CHECK (Dynamic Taming)
        // If humans (canBuild=true) are in the same cell as domesticatable animals
        // Note: simplified model - usually humans are a different agent layer or faction ID.
        // Assuming 'factionID' presence implies human control here for now as requested by user logic:
        // "If humans (canBuild=true) are in the same cell..."
        // In this architecture, a cell has ONE speciesID. So humans and animals can't share a cell 
        // unless we check neighbors or separate layers.
        // I'll implement the user's logic exactly as a placeholder, perhaps implied "Nearby" or "Faction Presence".
        // Let's check `b.factionID`. If factionID != 0, it means humans/civ owns the tile.
        
        bool humanPresence = (b.factionID && b.factionID[i] != 0);
        
        if (humanPresence && dna.isDomesticatable && !dna.isStatic) {
            // CheckForTaming needs to be moved to AgentSystem method or helper if not inline
            // User requested CheckForTaming private method.
            // I'll inline call to private method (exposed via friend or definition).
            // Actually, I can just define it here in the source as per C++ ODR if not defining class in header.
            // But AgentSystem is in header. I need to declare it in header or use private helper.
            // I will implement the logic here using a helper func.
            
            if (dna.aggression < 0.3f && dna.sociality > 0.6f) {
                // Taming Logic
                 std::cout << "[AGENT] Taming check passed for " << dna.speciesName << " at " << i << "\n";
                 // In a real system, we'd add it to the Faction's tamed list.
            }
        }

        // 3. CIVILIZATION EXPANSION
        if (dna.canBuild && b.population[i] > 1000) {
            // Faction claims territory and builds based on weights
            if (b.infrastructure) {
                 b.infrastructure[i] += 0.01f * deltaTime;
            }
        }
    }
}

void AgentSystem::HandleConstruction(uint32_t cellID, WorldBuffers& buffers, AgentTemplate& dna) {
    // Legacy/Helper if needed
}

void AgentSystem::HandleBorderExpansion(uint32_t cellID, WorldBuffers& buffers, AgentTemplate& dna) {
    // Legacy/Helper if needed
}
