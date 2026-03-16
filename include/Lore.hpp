#pragma once
#include "imgui.h"
#include "SagaConfig.hpp"
#include "WorldEngine.hpp"
#include <fstream>
#include <map>
#include <string>
#include <vector>

// --- ENUMS ---
enum class FieldType { TEXT, NUMBER, SLIDER, IMAGE_PATH, SPRITE_REF, COLOR };
enum class SocialType { SOLITARY, PACK, TRIBAL, NOMADIC, HIVE };

enum class ResourceRel {
  NONE = 0,
  NECESSITY,       // Critical Seek, Death if supply is 0
  ACTIVE_BONUS,    // Aggressive Seek, Buffs Pop Growth
  PASSIVE_BONUS,   // No seeking, Local Buff
  DESIRED_HELPFUL, // High Seek, High Expansion Buff
  USEFUL_HELPFUL,  // Minor Seek, Minor Expansion Buff
  WAR_DISLIKE,     // Seek & Destroy, Aggression Buff if neighbors have it
  PASSIVE_DISLIKE, // Destroy if found, but doesn't seek
  BUILD_REQUIRED,  // Industrial Seek, Scaling Penalty if 0
  BUILD_BONUS      // Minor Seek, Faster Build Speed
};

// --- CONSTANTS ---
const float SAGA_TEMP_MIN = -50.0f;
const float SAGA_TEMP_MAX = 50.0f;

// --- STRUCTURES ---
struct FieldDef {
  std::string label;
  FieldType type;
};

struct CategoryTemplate {
  std::string name;
  std::vector<FieldDef> fields;
};

struct WikiArticle {
  int id;
  std::string title;
  std::string categoryName; // Links to CategoryTemplate
  std::string content;      // Main Body (Markdown)

  // Simulation Toggles
  bool isAgent = false;
  bool isBiome = false;
  bool isFaction = false;
  bool isResource = false;

  // --- AGENT DNA ---
  AgentType agentType = AgentType::FAUNA;
  SocialType socialType = SocialType::SOLITARY;
  float minTemp = 0.0f, maxTemp = 1.0f;
  float minMoisture = 0.0f, maxMoisture = 1.0f;
  float expansion = 0.1f;
  float aggression = 0.1f;
  bool isTameable = false;
  bool isFarmable = false;

  // Resource Relationships: Key=ResourceID, Value=RelationshipType
  std::map<int, ResourceRel> resourceRelationships;

  // --- RESOURCE DATA ---
  float scarcity = 0.5f;          // 0=Mythic, 1=Abundant
  bool isRenewable = true;        // Regrows vs Finite
  std::vector<int> spawnBiomeIDs; // Biomes where it occurs
  // Biome preference: Key=BiomeID, Value=(-1 to 1)
  std::map<int, float> biomePreferences;
  // Production: Key=ResourceID, Value=Rate
  std::map<int, float> harvestOutput;
  std::map<int, float> livingOutput;

  // --- FACTION / CULTURE DATA ---
  int formationYear = 0;
  int formationCell = -1;
  std::vector<int> linkedEventIDs;

  // --- BIOME DATA ---
  float biomeTempMod = 0.0f;
  float biomeMoistureMod = 0.0f;
  ImVec4 biomeColor = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);

  // Dynamic Data (Matches Template)
  std::map<std::string, std::string> data;

  // Links
  std::vector<std::string> tags;
  int simID = -1; // Links to AgentSystem (Legacy/Direct)
  bool hasLocation = false;
  int mapX = 0, mapY = 0;

  // Media
  std::string imagePath;         // Main illustration
  unsigned int imageTexture = 0; // GPU Handle (GLuint)
};

// --- LOGGING ---
struct LogEntry {
  std::string timestamp;
  std::string category;
  std::string message;
};

#include "nlohmann/json.hpp"
// Using namespace for brevity in this header
using json = nlohmann::json;

class LoreScribe {
private:
  std::ofstream historyFile;
  std::vector<LogEntry> sessionLogs;

public:
  void Initialize(std::string filename);
  void RecordEvent(const ChronosConfig &time, std::string category, int cellID,
                   std::string log);
  void RecordJSONEvent(const std::string &type, const json &data);
  const std::vector<LogEntry> &GetRecentLogs();
};

namespace LoreScribeNS {
void Initialize();
void LogEvent(int tick, const std::string &type, int location,
              const std::string &desc);
void LogJsonEvent(const std::string &type, const json &data);
void Close();
extern int currentYear;
} // namespace LoreScribeNS

namespace NameGenerator {
std::string GeneratePersonName();
std::string GenerateFactionName();
std::string GenerateCityName();
} // namespace NameGenerator

// --- LORE MANAGER (Centralized IO) ---
namespace LoreManager {
extern std::vector<WikiArticle> wikiDB;
extern std::vector<CategoryTemplate> templates;

void Load(const std::string &lorePath = SagaConfig::LORE_JSON,
          const std::string &templPath = SagaConfig::TEMPLATE_JSON);
void Save(const std::string &lorePath = SagaConfig::LORE_JSON,
          const std::string &templPath = SagaConfig::TEMPLATE_JSON);

WikiArticle *GetArticle(int id);
WikiArticle *GetArticleByTitle(const std::string &title);

// Validation
struct SyncError {
  int id;
  std::string title;
  std::string message;
};
std::vector<SyncError> ValidateSync(struct WorldBuffers &buffers);

// FLAS Export
struct GlobalState {
  int currentEpoch = 1;
  std::map<std::string, bool> worldFlags;
  std::vector<int> activeFactionIDs;
};
void ExportGlobalState(const std::string &path,
                       const struct WorldBuffers &buffers);
} // namespace LoreManager
