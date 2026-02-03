#include "../../include/SimulationModules.hpp"
#include "../../include/WorldEngine.hpp" // Needs full access to buffers

// Helper function to resolve battles
void ResolveConflicts(WorldBuffers& b, const NeighborGraph& graph) {

    // Iterate through all cells to check borders
    // Using b.count if available or hardcoded 1M (using b.count is safer)
    uint32_t count = b.count;
    if (count == 0) count = 1000000;

    for (uint32_t i = 0; i < count; ++i) {
        int myFaction = b.factionID[i];

        // Skip uninhabited land or water
        if (myFaction == 0 || b.height[i] < 0.2f) continue;

        float myDefense = b.population[i] + (b.infrastructure[i] * 500.0f);

        // Check Neighbors via the Graph
        int offset = graph.offsetTable[i];
        int n_count = graph.countTable[i];

        for (int n = 0; n < n_count; ++n) {
            int neighborIdx = graph.neighborData[offset + n];
            int theirFaction = b.factionID[neighborIdx];

            // If neighbor is an enemy (different faction ID)
            if (theirFaction != 0 && theirFaction != myFaction) {

                float theirAttack = b.population[neighborIdx] * 1.5f; // Aggression Bonus

                // BATTLE RESOLUTION
                if (theirAttack > myDefense * 1.2f) {
                    // They win! Border shift.
                    b.factionID[i] = theirFaction; // Territory flips
                    b.population[i] = (uint32_t)(b.population[i] * 0.5f);       // Casualties
                    b.chaos[i] += 0.2f;            // War creates Chaos

                    // Note: Here is where you would call LoreScribe::RecordEvent()
                }
            }
        }
    }
}
