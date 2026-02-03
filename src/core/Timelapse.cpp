#include "../../include/SimulationModules.hpp"

void TimelapseManager::TagRegion(uint32_t cellID, int r) {
    taggedCellStart = cellID;
    radius = r;
    history.clear();
}

void TimelapseManager::TakeSnapshot(const WorldBuffers& b, const ChronosConfig& time) {
    // Only take a snapshot once a month to save memory
    Snapshot s;
    s.year = time.yearCount;
    
    // We only save the data for the tagged square
    // Assuming simple X/Y grid logic where Width=Sqrt(Count)
    int side = 0;
    // We don't have side stored explicitly locally, but usually map is square
    // Rough estimation
    if (b.count > 0) side = (int)std::sqrt(b.count);
    else return;

    if (taggedCellStart >= b.count) return;
    
    // Determine Center X/Y
    int cX = taggedCellStart % side;
    int cY = taggedCellStart / side;

    for (int y = -radius; y < radius; ++y) {
        for (int x = -radius; x < radius; ++x) {
            int targetX = cX + x;
            int targetY = cY + y;
            
            if (targetX >= 0 && targetX < side && targetY >= 0 && targetY < side) {
                int target = targetX + (targetY * side);
                if (target >= 0 && target < (int)b.count) {
                    if(b.factionID) s.factionData.push_back(b.factionID[target]);
                    if(b.population) s.popData.push_back((float)b.population[target]);
                }
            }
        }
    }
    history.push_back(s);
}
