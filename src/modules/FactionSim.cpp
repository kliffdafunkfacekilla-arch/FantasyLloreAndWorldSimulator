#include "../../include/SimulationModules.hpp"
#include <iostream>

// Implementation of Faction Logic
namespace FactionLogic {

    void ProcessDomestication(Faction& fact, AgentInstance& animal, const std::vector<AgentTemplate>& registry) {
        if (animal.speciesID < 0 || animal.speciesID >= (int)registry.size()) return;

        const auto& dna = registry[animal.speciesID];
        
        // The "Dynamic Taming" Formula
        // Low aggression, High sociality required
        if (dna.aggression < 0.3f && dna.sociality > 0.6f) {
            
            // Check if already tamed to avoid duplicates
            bool known = false;
            for(int id : fact.assets.tamedSpeciesIDs) {
                if(id == animal.speciesID) { known = true; break; }
            }

            if (!known) {
                fact.assets.tamedSpeciesIDs.push_back(animal.speciesID);
                std::cout << "[FACTION] " << fact.name << " domesticated " << dna.speciesName << "!\n";
                
                // Apply passive bonus
                if (dna.size > 0.8f) {
                    fact.assets.transportSpeed += 0.5f; 
                    std::cout << "          Transport Speed increased!\n";
                }
            }
        }
    }

} // namespace
