#include "../../include/SimulationModules.hpp"
#include <iostream>

void LifeSim::SeedSpecies(WorldBuffers& buffers, const AgentTemplate& dna, uint32_t startCell) {
    if (!buffers.population || !buffers.speciesID) {
        std::cout << "[LIFE] Error: Buffers not allocated for life.\n";
        return;
    }
    
    // Place initial population
    buffers.population[startCell] = 100;
    // Mark which DNA/Species is in this cell
    buffers.speciesID[startCell] = dna.id; 
    
    std::cout << "[LIFE] Spawning " << dna.speciesName << " at cell " << startCell << ".\n";
}

void LifeSim::Evolve(AgentTemplate& dna, float cellTemp, float cellMoisture) {
    if (dna.adaptiveRate <= 0) return;

    // Drift the ideal toward the current environment
    float tempDiff = cellTemp - dna.idealTemp;
    dna.idealTemp += tempDiff * dna.adaptiveRate;

    float moistureDiff = cellMoisture - dna.idealMoisture;
    dna.idealMoisture += moistureDiff * dna.adaptiveRate;
    
    // Debug output periodically? simplified.
}
