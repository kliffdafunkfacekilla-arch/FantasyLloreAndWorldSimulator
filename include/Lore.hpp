#pragma once
#include "WorldEngine.hpp"
#include <fstream>
#include <string>
#include <vector>


// Helper struct for LoreScribe
struct LogEntry {
  std::string timestamp;
  std::string category;
  std::string message;
};

// Lore & History (src/lore/LoreScribe.cpp)
class LoreScribe {
private:
  std::ofstream historyFile;
  std::vector<LogEntry> sessionLogs;

public:
  void Initialize(std::string filename);
  void RecordEvent(const ChronosConfig &time, std::string category, int cellID,
                   std::string log);
  const std::vector<LogEntry> &GetRecentLogs();
};

// LoreScribe namespace (simpler API)
namespace LoreScribeNS {
void Initialize();
void LogEvent(int tick, const std::string &type, int location,
              const std::string &desc);
void Close();
extern int currentYear;
} // namespace LoreScribeNS

// NameGenerator namespace (src/lore/NameGenerator.cpp)
namespace NameGenerator {
std::string GeneratePersonName();
std::string GenerateFactionName();
std::string GenerateCityName();
} // namespace NameGenerator
