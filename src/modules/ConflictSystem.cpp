#include "../../include/SimulationModules.hpp"
#include <algorithm>
#include <cmath>
#include <string>

// Helper helpers
bool FactionHasWarBeastConf(const Faction& f, const std::vector<AgentTemplate>& registry) {
    for (int sID : f.assets.tamedSpeciesIDs) {
        if (sID >= 0 && sID < (int)registry.size()) {
            const auto& dna = registry[sID];
            if (dna.aggression > 0.7f && dna.size > 0.5f) return true;
        }
    }
    return false;
}

const Faction* GetFactionConf(int id, const std::vector<Faction>& factions) {
    for(const auto& f : factions) if(f.id == id) return &f;
    return nullptr;
}

std::string GetFactionName(int id, const std::vector<Faction>& factions) {
    const Faction* f = GetFactionConf(id, factions);
    return f ? f->name : "Unknown Logic";
}

void ConflictSystem::ResolveBorders(WorldBuffers& b, const std::vector<AgentTemplate>& registry, 
                                    const std::vector<Faction>& factions, const NeighborGraph& graph, 
                                    LoreScribe& scribe, const ChronosConfig& time, float deltaTime) {
    
    if (!b.population || !b.factionID || !b.infrastructure) return;

    for (uint32_t i = 0; i < b.count; ++i) {
        if (b.factionID[i] == 0) continue; 

        float defense = b.population[i] * 0.1f;
        defense += b.infrastructure[i] * 2.0f; 

        if (b.resourceLevels) {
             float supplyLine = b.resourceLevels[i * 4 + 0]; // Sim
             if (supplyLine < 0.1f) {
                 defense *= 0.5f; 
             }
        }

        if (graph.offsetTable && graph.countTable) {
            int start = graph.offsetTable[i];
            int count = graph.countTable[i];

            for (int k = 0; k < count; ++k) {
                int neighbor = graph.neighborData[start + k];
                
                int enemyID = b.factionID[neighbor];
                if (enemyID != 0 && enemyID != b.factionID[i]) {
                    
                    float attack = b.population[neighbor] * 0.15f;
                    
                    const Faction* enemyFaction = GetFactionConf(enemyID, factions);
                    if (enemyFaction && FactionHasWarBeastConf(*enemyFaction, registry)) {
                        attack *= 1.5f; 
                    }

                    if (attack > defense) {
                        // Check for Major Battle (Lore)
                        // Trigger logic: Overwhelming force and significant population involved
                        if (attack > defense * 1.5f && b.population[i] > 1000) {
                            std::string attackerName = enemyFaction ? enemyFaction->name : "Rebels";
                            std::string defenderName = GetFactionName(b.factionID[i], factions);
                            
                            std::string battleDesc = "The " + attackerName + 
                             " decimated the " + defenderName + " defenses at cell " + std::to_string(i) + ".";
                             
                            scribe.RecordEvent(time, "WAR", i, battleDesc);
                            
                            if (b.chaosEnergy) b.chaosEnergy[i] += 0.5f;
                        }

                        // Conquest
                        b.factionID[i] = enemyID; 
                        b.population[i] *= 0.8f; 
                        b.infrastructure[i] *= 0.5f; 
                        
                        break; 
                    }
                }
            }
        }
    }
}
