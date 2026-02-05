#include "../../include/SimulationModules.hpp"
#include <iostream>

void LoreScribe::Initialize(std::string filename) {
    // Open in Append mode so we don't wipe history on restart
    historyFile.open(filename, std::ios::app);

    // Write Header if file is empty/new
    if (historyFile.tellp() == 0) {
        historyFile << "Year,Month,Day,Category,CellID,Log\n";
    }
    std::cout << "[IO] Lore Scribe initialized: " << filename << std::endl;
}

void LoreScribe::RecordEvent(const ChronosConfig& time, std::string category, int cellID, std::string text) {
    // 1. Write to Disk (CSV format)
    // Format: Year, Month, Day, Category, CellID, "Message"
    historyFile << time.yearCount << "," << time.monthCount << "," << time.dayCount << ","
                << category << "," << cellID << ",\"" << text << "\"\n";

    // Flush to ensure external Lore Apps see it immediately
    historyFile.flush();

    // 2. Write to RAM (for In-Game UI)
    std::string date = "Y" + std::to_string(time.yearCount) + "-M" + std::to_string(time.monthCount);
    sessionLogs.push_back({date, category, text});

    // Keep buffer small (last 50 events) to save RAM
    if (sessionLogs.size() > 50) {
        sessionLogs.erase(sessionLogs.begin());
    }
}

// Helper for the Dashboard to read recent events
const std::vector<LogEntry>& LoreScribe::GetRecentLogs() {
    return sessionLogs;
}
