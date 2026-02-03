#include "../../include/SimulationModules.hpp"
#include <iostream>

void LoreScribe::Initialize(std::string worldName) {
    historyFile.open(worldName + "_history.csv", std::ios::app);
    if (historyFile.is_open()) {
         historyFile.seekp(0, std::ios::end);
         if (historyFile.tellp() == 0) {
            historyFile << "Year,Month,Day,Category,CellID,Log\n";
         }
    }
}

void LoreScribe::RecordEvent(const ChronosConfig& time, std::string cat, int cellID, std::string text) {
    // Write to CSV for external lore apps
    if (historyFile.is_open()) {
        historyFile << time.yearCount << "," << time.monthCount << "," << time.dayCount << ","
                    << cat << "," << cellID << ",\"" << text << "\"\n";
        historyFile.flush();
    }
    
    // Keep a small buffer for the in-engine UI
    LoreEvent e = {time.yearCount, time.monthCount, time.dayCount, cat, cellID, text};
    sessionLogs.push_back(e);
    if (sessionLogs.size() > 50) sessionLogs.erase(sessionLogs.begin());
    
    // Console Echo
    std::cout << "[LORE][" << cat << "] " << text << "\n";
}

const std::vector<LoreEvent>& LoreScribe::GetSessionLogs() { 
    return sessionLogs; 
}
