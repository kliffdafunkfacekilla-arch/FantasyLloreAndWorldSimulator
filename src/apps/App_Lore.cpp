#include "../../include/AssetManager.hpp"
#include "../../include/BinaryExporter.hpp"
#include "../../include/Lore.hpp"
#include "../../include/PlatformUtils.hpp"
#include "../../include/SagaConfig.hpp"
#include "../../include/WorldEngine.hpp"
#include "../../include/nlohmann/json.hpp"
#include "../../include/stb_image.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "../../deps/imgui/backends/imgui_impl_glfw.h"
#include "../../deps/imgui/backends/imgui_impl_opengl3.h"
#include "imgui.h"
#include "../../deps/imgui/misc/cpp/imgui_stdlib.h"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#if defined(__GNUC__) && (__GNUC__ < 8) && !defined(__clang__)
namespace fs = std::experimental::filesystem;
#else
namespace fs = std::filesystem;
#endif

// --- THEME ---
void SetupTheme() {
  ImGuiStyle &style = ImGui::GetStyle();
  style.WindowRounding = 6.0f;
  style.FrameRounding = 4.0f;
  style.Colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.13f, 1.00f);
  style.Colors[ImGuiCol_Header] = ImVec4(0.20f, 0.25f, 0.30f, 1.00f);
  style.Colors[ImGuiCol_Button] = ImVec4(0.25f, 0.40f, 0.55f, 1.00f);
  style.Colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
}

// --- ADVANCED CALENDAR SYSTEM ---
struct Moon {
  std::string name = "Luna";
  float cycle = 29.5f; // Days per cycle
  float shift = 0.0f;  // Offset
  ImVec4 color = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);
};

struct Season {
  std::string name = "Summer";
  int startMonth = 6;
  int startDay = 1;
  ImVec4 color = ImVec4(1.0f, 0.6f, 0.2f, 1.0f);
};

struct Era {
  std::string name;
  std::string abbreviation; // e.g. "AD", "BC", "3E"
  int startYear;
  bool inverted = false; // For BC dates counting down
};

// --- SMART IMPORT STRUCTURES ---
struct Suggestion {
  std::string name;
  std::string type; // AGENT, FACTION, RESOURCE
  std::string source;
};
std::vector<Suggestion> importQueue;

void LoadSuggestions() {
  std::string sugPath = SagaConfig::DATA_HUB + "suggestions.json";
  std::ifstream f(sugPath);
  if (f.is_open()) {
    importQueue.clear();
    try {
      nlohmann::json j;
      f >> j;
      for (const auto &item : j) {
        Suggestion s;
        s.name = item.value("name", "Unknown");
        s.type = item.value("type", "UNKNOWN");
        s.source = item.value("source", "");
        importQueue.push_back(s);
      }
    } catch (...) {
    }
  }
}

struct CalendarSystem {
  // Basics
  std::string name = "Gregorian";
  int currentYear = 1;
  int currentMonth = 1;
  int currentDay = 1;

  // Time
  int hoursPerDay = 24;
  int minutesPerHour = 60;

  // Structure
  std::vector<std::string> monthNames = {
      "Hammer",    "Alturiak", "Ches",   "Tarsakh",   "Mirtul", "Kythorn",
      "Flamerule", "Eleasis",  "Eleint", "Marpenoth", "Uktar",  "Nightal"};
  std::vector<int> monthLengths; // Days per month
  std::vector<std::string> weekDayNames = {"Monday",   "Tuesday", "Wednesday",
                                           "Thursday", "Friday",  "Saturday",
                                           "Sunday"};

  // Advanced Features
  std::vector<Moon> moons;
  std::vector<Season> seasons;
  std::vector<Era> eras;

  CalendarSystem() {
    // Defaults
    for (int i = 0; i < 12; ++i)
      monthLengths.push_back(30);
    moons.push_back({"Selune", 30.44f, 0.0f, ImVec4(0.8f, 0.8f, 1.0f, 1.0f)});
  }

  void Save() {
    nlohmann::json j;
    j["name"] = name;
    j["current"] = {currentYear, currentMonth, currentDay};
    j["hoursPerDay"] = hoursPerDay;
    j["minutesPerHour"] = minutesPerHour;
    j["months"] = monthNames;
    j["monthLens"] = monthLengths;
    j["weekdays"] = weekDayNames;

    // Moons
    nlohmann::json mArr = nlohmann::json::array();
    for (auto &m : moons)
      mArr.push_back({{"n", m.name},
                      {"c", m.cycle},
                      {"s", m.shift},
                      {"col", {m.color.x, m.color.y, m.color.z}}});
    j["moons"] = mArr;

    // Seasons
    nlohmann::json sArr = nlohmann::json::array();
    for (auto &s : seasons)
      sArr.push_back({{"n", s.name}, {"m", s.startMonth}, {"d", s.startDay}});
    j["seasons"] = sArr;

    // Eras
    nlohmann::json eArr = nlohmann::json::array();
    for (auto &e : eras)
      eArr.push_back({{"n", e.name},
                      {"a", e.abbreviation},
                      {"y", e.startYear},
                      {"i", e.inverted}});
    j["eras"] = eArr;

    std::ofstream o(SagaConfig::CALENDAR_JSON);
    o << std::setw(4) << j;
  }

  void Load() {
    std::ifstream f(SagaConfig::CALENDAR_JSON);
    if (f.is_open()) {
      try {
        nlohmann::json j;
        f >> j;
        name = j.value("name", "New Calendar");
        hoursPerDay = j.value("hoursPerDay", 24);
        minutesPerHour = j.value("minutesPerHour", 60);

        if (j.contains("current") && j["current"].is_array() &&
            j["current"].size() >= 3) {
          currentYear = j["current"][0];
          currentMonth = j["current"][1];
          currentDay = j["current"][2];
        }

        if (j.contains("months")) {
          monthNames = j["months"].get<std::vector<std::string>>();
          monthLengths = j["monthLens"].get<std::vector<int>>();
        }
        if (j.contains("weekdays")) {
          weekDayNames = j["weekdays"].get<std::vector<std::string>>();
        }
        if (j.contains("moons")) {
          moons.clear();
          for (auto &obj : j["moons"]) {
            std::vector<float> c = obj["col"].get<std::vector<float>>();
            moons.push_back(
                {obj["n"], obj["c"], obj["s"], ImVec4(c[0], c[1], c[2], 1)});
          }
        }
        if (j.contains("seasons")) {
          seasons.clear();
          for (auto &obj : j["seasons"]) {
            seasons.push_back({obj["n"], obj["m"], obj["d"]});
          }
        }
        if (j.contains("eras")) {
          eras.clear();
          for (auto &obj : j["eras"]) {
            eras.push_back(
                {obj["n"], obj["a"], obj["y"], obj.value("i", false)});
          }
        }
      } catch (...) {
      }
    }
  }
};

// --- DATA STRUCTURES ---
// (Now using centrally defined structures in Lore.hpp)

// --- GLOBALS ---
CalendarSystem calendar;
#define wikiDB LoreManager::wikiDB
#define templates LoreManager::templates

static bool useFahrenheit = false;

float ToCelsius(float n) { return (n * 100.0f) - 50.0f; }
float FromCelsius(float c) { return (c + 50.0f) / 100.0f; }
float ToFahrenheit(float c) { return c * 1.8f + 32.0f; }
float FromFahrenheit(float f) { return (f - 32.0f) / 1.8f; }

WorldBuffers mapData;
GLuint mapTexture = 0;
int selectedIdx = -1;
bool showMapOverlay = false;
bool showHelp = false;

// --- HELPER: BIOME NAMES ---
const char *GetBiomeName(int id) {
  switch (id) {
  case 0:
    return "Deep Ocean";
  case 1:
    return "Ocean";
  case 2:
    return "Beach";
  case 3:
    return "Scorched";
  case 4:
    return "Desert";
  case 5:
    return "Savanna";
  case 6:
    return "Tropical Rainforest";
  case 7:
    return "Grassland";
  case 8:
    return "Forest";
  case 9:
    return "Temperate Rainforest";
  case 10:
    return "Taiga";
  case 11:
    return "Tundra";
  case 12:
    return "Snow";
  case 13:
    return "Mountain";
  default:
    return "Unknown Biome";
  }
}

// --- HELPER: IMAGE LOADER ---
GLuint LoadTexture(const std::string &path) {
  if (path.empty() || !fs::exists(path))
    return 0;
  int w, h, ch;
  unsigned char *data = stbi_load(path.c_str(), &w, &h, &ch, 4);
  if (!data)
    return 0;

  GLuint tex;
  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D, tex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE,
               data);
  stbi_image_free(data);
  return tex;
}

// --- HELPER: MARKDOWN RENDERER ---
void RenderMarkdown(const std::string &text) {
  std::stringstream ss(text);
  std::string line;
  while (std::getline(ss, line)) {
    if (line.rfind("# ", 0) == 0) { // H1
      ImGui::SetWindowFontScale(1.5f);
      ImGui::TextColored(ImVec4(1, 0.8f, 0.4f, 1), "%s",
                         line.substr(2).c_str());
      ImGui::SetWindowFontScale(1.0f);
      ImGui::Separator();
    } else if (line.rfind("## ", 0) == 0) { // H2
      ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1), "%s",
                         line.substr(3).c_str());
    } else if (line.rfind("* ", 0) == 0) { // Bullet
      ImGui::Bullet();
      ImGui::TextWrapped("%s", line.substr(2).c_str());
    } else if (line.rfind("---", 0) == 0) { // Divider
      ImGui::Separator();
    } else { // Body
      ImGui::TextWrapped("%s", line.c_str());
    }
  }
}

// --- IO SYSTEMS ---
void SaveData() {
  LoreManager::Save();
  AssetManager::SaveAll(); // Save Sim Rules
  calendar.Save();
}

void LoadData() {
  AssetManager::Initialize();
  LoreManager::Load();
  AssetManager::LoadAll();
  calendar.Load();
}

// --- EXPORT TO HTML ---
void ExportHTML() {
  fs::create_directories("export");

  // Index
  std::ofstream idx("export/index.html");
  idx << "<html><head><style>body{background:#222;color:#eee;font-family:sans-"
         "serif;max-width:800px;margin:auto;padding:20px;} "
         "a{color:#4da6ff;}</style></head><body>";
  idx << "<h1>" << calendar.name << " World Codex</h1><hr>";

  for (const auto &a : wikiDB) {
    std::string fname = "article_" + std::to_string(a.id) + ".html";
    idx << "<h3><a href='" << fname << "'>" << a.title << "</a> <small>("
        << a.categoryName << ")</small></h3>";

    // Article Page
    std::ofstream p("export/" + fname);
    p << "<html><head><style>body{background:#222;color:#eee;font-family:sans-"
         "serif;max-width:800px;margin:auto;padding:20px;}</style></"
         "head><body>";
    p << "<a href='index.html'>&larr; Back</a>";
    p << "<h1>" << a.title << "</h1>";
    if (a.hasLocation)
      p << "<p><b>Location:</b> " << a.mapX << ", " << a.mapY << "</p>";

    // Data Table
    if (!a.data.empty()) {
      p << "<table border='1' style='border-collapse:collapse;width:100%'>";
      for (auto &[k, v] : a.data)
        p << "<tr><td style='padding:5px'><b>" << k
          << "</b></td><td style='padding:5px'>" << v << "</td></tr>";
      p << "</table><br>";
    }

    // Content (Simple markdown conversion)
    std::string htmlContent = a.content;
    p << "<div style='white-space: pre-wrap;'>" << htmlContent << "</div>";
    p << "</body></html>";
  }
  idx << "</body></html>";
  system("start export/index.html");
}

// --- UI COMPONENTS ---

void DrawSidebar() {
  ImGui::Begin("Codex", nullptr, ImGuiWindowFlags_NoCollapse);
  if (ImGui::Button("+ New Article", ImVec2(-1, 0))) {
    WikiArticle a;
    a.id = rand();
    a.title = "New";
    a.categoryName = "General";
    wikiDB.push_back(a);
    selectedIdx = (int)wikiDB.size() - 1;
  }

  if (ImGui::Button("Scan Lore (Generate Suggestions)", ImVec2(-1, 0))) {
    // Robust path resolution: find scripts/ relative to the executable
    fs::path exeDir = PlatformUtils::GetExecutablePath();
    fs::path projectRoot = exeDir.parent_path();
    fs::path script = projectRoot / "scripts" / "lore_scanner.py";

    // Build command: normalize to backslashes for Windows shell
    std::string scriptPath = script.make_preferred().string();
    fs::path hubPath = SagaConfig::DATA_HUB;
    if (hubPath.has_filename()) {
      // No trailing slash
    } else {
      hubPath = hubPath.parent_path();
    }
    std::string dataDir = hubPath.make_preferred().string();

    std::string cmd =
        "python \"" + scriptPath + "\" --data-dir \"" + dataDir + "\"";
    std::cout << "[SCAN] " << cmd << std::endl;
    int result = system(cmd.c_str());
    if (result != 0) {
      // Try "py" if "python" fails
      cmd = "py \"" + scriptPath + "\" --data-dir \"" + dataDir + "\"";
      system(cmd.c_str());
    }

    // Reload the tagged lore data
    LoreManager::Load();
    LoadSuggestions();
    AssetManager::SyncWithLore();
    ImGui::OpenPopup("SmartImportPopup");
  }

  // --- SMART IMPORT POPUP ---
  if (ImGui::BeginPopup("SmartImportPopup")) {
    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1), "Found %d Entities",
                       (int)importQueue.size());
    ImGui::Separator();
    ImGui::BeginChild("ScrollSugg", ImVec2(400, 300));

    for (auto it = importQueue.begin(); it != importQueue.end();) {
      ImGui::PushID(it->name.c_str());
      ImGui::Text("%s (%s)", it->name.c_str(), it->type.c_str());
      bool processed = false;

      if (ImGui::Button("Add Agent")) {
        WikiArticle a;
        a.id = rand();
        a.title = it->name;
        a.categoryName = "Bestiary";
        a.isAgent = true;
        wikiDB.push_back(a);
        processed = true;
      }
      ImGui::SameLine();
      if (ImGui::Button("Add Faction")) {
        WikiArticle a;
        a.id = rand();
        a.title = it->name;
        a.categoryName = "Faction";
        a.isFaction = true;
        wikiDB.push_back(a);
        processed = true;
      }
      ImGui::SameLine();
      if (ImGui::Button("Add Resource")) {
        WikiArticle a;
        a.id = rand();
        a.title = it->name;
        a.categoryName = "Resource";
        a.isResource = true;
        wikiDB.push_back(a);
        processed = true;
      }
      ImGui::SameLine();
      if (ImGui::Button("Ignore")) {
        processed = true;
      }

      if (processed) {
        it = importQueue.erase(it);
        ImGui::PopID();
        continue;
      }
      ImGui::PopID();
      ++it;
    }
    ImGui::EndChild();
    ImGui::EndPopup();
  }
  AssetManager::SyncWithLore();

  ImGui::Separator();

  static char filter[64] = "";
  ImGui::InputText("Search", filter, 64);

  // Group by Category
  std::map<std::string, std::vector<int>> categories;
  for (int i = 0; i < (int)wikiDB.size(); ++i) {
    bool match = false;
    if (strlen(filter) == 0)
      match = true;
    else {
      if (wikiDB[i].title.find(filter) != std::string::npos)
        match = true;
      if (wikiDB[i].categoryName.find(filter) != std::string::npos)
        match = true;
      for (const auto &t : wikiDB[i].tags) {
        if (t.find(filter) != std::string::npos)
          match = true;
      }
    }

    if (!match)
      continue;
    categories[wikiDB[i].categoryName].push_back(i);
  }

  ImGui::BeginChild("List");
  for (auto const &[catName, indices] : categories) {
    if (ImGui::CollapsingHeader(catName.c_str(),
                                ImGuiTreeNodeFlags_DefaultOpen)) {
      for (int idx : indices) {
        ImVec4 col = ImVec4(1, 1, 1, 1);
        if (wikiDB[idx].categoryName == "Faction")
          col = ImVec4(1, 0.6f, 0.6f, 1);

        ImGui::PushStyleColor(ImGuiCol_Text, col);
        if (ImGui::Selectable(wikiDB[idx].title.c_str(), selectedIdx == idx))
          selectedIdx = idx;
        ImGui::PopStyleColor();
      }
    }
  }
  ImGui::EndChild();
  ImGui::End();
}

void DrawTemplateEditor() {
  if (!ImGui::Begin("Category Builder")) {
    ImGui::End();
    return;
  }

  static int selTemp = 0;
  if (templates.empty()) {
    ImGui::Text("No Templates");
    ImGui::End();
    return;
  }
  if (selTemp >= templates.size())
    selTemp = 0;

  if (ImGui::BeginCombo("Template", templates[selTemp].name.c_str())) {
    for (int i = 0; i < templates.size(); ++i)
      if (ImGui::Selectable(templates[i].name.c_str(), selTemp == i))
        selTemp = i;
    ImGui::EndCombo();
  }
  ImGui::SameLine();
  if (ImGui::Button("+")) {
    templates.push_back({"New Category", {}});
    selTemp = (int)templates.size() - 1;
  }

  ImGui::InputText("Name", &templates[selTemp].name);
  ImGui::Separator();
  ImGui::Text("Fields:");

  auto &fields = templates[selTemp].fields;
  for (int i = 0; i < fields.size(); ++i) {
    ImGui::PushID(i);
    ImGui::SetNextItemWidth(100);
    ImGui::InputText("##lbl", &fields[i].label);
    ImGui::SameLine();
    const char *types[] = {"Text",  "Number", "Slider",
                           "Color", "Image",  "Sprite"};
    int t = (int)fields[i].type;
    ImGui::SetNextItemWidth(80);
    if (ImGui::Combo("##type", &t, types, 6))
      fields[i].type = (FieldType)t;
    ImGui::SameLine();
    if (ImGui::Button("x")) {
      fields.erase(fields.begin() + i);
      ImGui::PopID();
      break;
    }
    ImGui::PopID();
  }
  if (ImGui::Button("Add Field"))
    fields.push_back({"New Field", FieldType::TEXT});

  ImGui::End();
}

void DrawMainEditor() {
  ImGui::Begin("Inspector");
  if (selectedIdx >= 0 && selectedIdx < wikiDB.size()) {
    WikiArticle &a = wikiDB[selectedIdx];

    ImGui::InputText("##Title", &a.title);
    ImGui::SameLine();
    if (ImGui::Button("Export HTML"))
      ExportHTML();
    ImGui::SameLine();
    ImGui::Checkbox("°F", &useFahrenheit);
    ImGui::SameLine();
    if (ImGui::Button("?"))
      showHelp = true;

    ImGui::Separator();
    if (ImGui::CollapsingHeader("Simulation Identity",
                                ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::Checkbox("Is Simulation Agent", &a.isAgent);
      ImGui::SameLine();
      ImGui::Checkbox("Is Custom Biome", &a.isBiome);
      ImGui::SameLine();
      ImGui::Checkbox("Is Faction/Culture", &a.isFaction);
      ImGui::SameLine();
      ImGui::Checkbox("Is Natural Resource", &a.isResource);
    }

    if (a.isAgent) {
      if (ImGui::CollapsingHeader("Agent DNA (Biology & Behavior)",
                                  ImGuiTreeNodeFlags_DefaultOpen)) {
        // --- BASIC TYPE ---
        const char *typeNames[] = {"Flora", "Fauna", "Civilized"};
        int typeInt = (int)a.agentType;
        if (ImGui::Combo("Biological Class", &typeInt, typeNames, 3))
          a.agentType = (AgentType)typeInt;

        const char *socialNames[] = {"Solitary", "Pack", "Tribal", "Nomadic",
                                     "Hive"};
        int socInt = (int)a.socialType;
        if (ImGui::Combo("Social Structure", &socInt, socialNames, 5))
          a.socialType = (SocialType)socInt;

        ImGui::Separator();

        // --- ENVIRONMENTAL LIMITS ---
        ImGui::Text("Environmental Tolerance");
        float tMin = useFahrenheit ? ToFahrenheit(ToCelsius(a.minTemp))
                                   : ToCelsius(a.minTemp);
        float tMax = useFahrenheit ? ToFahrenheit(ToCelsius(a.maxTemp))
                                   : ToCelsius(a.maxTemp);
        const char *unit = useFahrenheit ? "°F" : "°C";

        ImGui::SetNextItemWidth(200);
        if (ImGui::DragFloatRange2(unit, &tMin, &tMax, 0.5f,
                                   useFahrenheit ? -58.0f : -50.0f,
                                   useFahrenheit ? 122.0f : 50.0f)) {
          float cMin = useFahrenheit ? FromFahrenheit(tMin) : tMin;
          float cMax = useFahrenheit ? FromFahrenheit(tMax) : tMax;
          a.minTemp = FromCelsius(cMin);
          a.maxTemp = FromCelsius(cMax);
        }

        const char *moistureLabels[] = {"Arid", "Semi-Arid", "Moderate",
                                        "Humid", "Saturated"};
        int mMinTier = (int)(a.minMoisture * 4.0f);
        int mMaxTier = (int)(a.maxMoisture * 4.0f);
        ImGui::SetNextItemWidth(100);
        if (ImGui::Combo("Min Moisture", &mMinTier, moistureLabels, 5))
          a.minMoisture = mMinTier / 4.0f;
        ImGui::SameLine();
        ImGui::SetNextItemWidth(100);
        if (ImGui::Combo("Max Moisture", &mMaxTier, moistureLabels, 5))
          a.maxMoisture = mMaxTier / 4.0f;

        ImGui::Separator();

        // --- BEHAVIOR ---
        const char *aggLabels[] = {"Pacifist", "Defensive", "Neutral",
                                   "Aggressive", "Warmonger"};
        int aggTier = (int)(a.aggression * 4.0f);
        if (ImGui::Combo("Aggression Level", &aggTier, aggLabels, 5))
          a.aggression = aggTier / 4.0f;

        const char *expLabels[] = {"Stagnant", "Slow", "Steady", "Rapid",
                                   "Invasive"};
        int expTier = (int)(a.expansion * 4.0f);
        if (ImGui::Combo("Expansion Rate", &expTier, expLabels, 5))
          a.expansion = expTier / 4.0f;

        ImGui::Checkbox("Tameable", &a.isTameable);
        ImGui::SameLine();
        ImGui::Checkbox("Farmable", &a.isFarmable);

        ImGui::Separator();

        // --- RESOURCE RELATIONSHIPS ---
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f),
                           "Resource Relationship Engine");
        if (ImGui::BeginTable("ResRels", 3, ImGuiTableFlags_Borders)) {
          ImGui::TableSetupColumn("Resource");
          ImGui::TableSetupColumn("Interaction Type");
          ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed,
                                  40);
          ImGui::TableHeadersRow();

          const char *relNames[] = {
              "None",          "Necessity",         "Active Bonus",
              "Passive Bonus", "Desired (Helpful)", "Useful (Helpful)",
              "War Dislike",   "Passive Dislike",   "Build Required",
              "Build Bonus"};

          for (auto it = a.resourceRelationships.begin();
               it != a.resourceRelationships.end();) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("%s",
                        AssetManager::resourceRegistry[it->first].name.c_str());
            ImGui::TableNextColumn();

            int rInt = (int)it->second;
            ImGui::SetNextItemWidth(-1);
            if (ImGui::Combo(("##rel" + std::to_string(it->first)).c_str(),
                             &rInt, relNames, 10)) {
              it->second = (ResourceRel)rInt;
            }

            ImGui::TableNextColumn();
            if (ImGui::Button(
                    ("X##delr" + std::to_string(it->first)).c_str())) {
              it = a.resourceRelationships.erase(it);
              continue;
            }
            ++it;
          }
          ImGui::EndTable();
        }

        if (ImGui::BeginCombo("##addRes", "Add Relationship...")) {
          for (int i = 0; i < (int)AssetManager::resourceRegistry.size(); ++i) {
            if (ImGui::Selectable(
                    AssetManager::resourceRegistry[i].name.c_str())) {
              a.resourceRelationships[i] = ResourceRel::PASSIVE_BONUS;
            }
          }
          ImGui::EndCombo();
        }

        ImGui::Separator();

        // --- BIOME PREFERENCES ---
        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.8f, 1.0f),
                           "Biome Love/Hate List");
        if (ImGui::BeginTable("BiomePrefs", 3, ImGuiTableFlags_Borders)) {
          ImGui::TableSetupColumn("Biome");
          ImGui::TableSetupColumn("Value (-1 Hate, +1 Love)");
          ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed,
                                  40);
          ImGui::TableHeadersRow();

          for (auto it = a.biomePreferences.begin();
               it != a.biomePreferences.end();) {
            ImGui::PushID(it->first);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("%s (%d)", GetBiomeName(it->first), it->first);
            ImGui::TableNextColumn();
            ImGui::SliderFloat("##val", &it->second, -1.0f, 1.0f);
            ImGui::TableNextColumn();
            if (ImGui::Button("X##delb")) {
              it = a.biomePreferences.erase(it);
              ImGui::PopID();
              continue;
            }
            ImGui::PopID();
            ++it;
          }
          ImGui::EndTable();
        }

        if (ImGui::BeginCombo("##addBiome", "Add Biome...")) {
          for (int i = 0; i < 25; ++i) {
            const char *bName = GetBiomeName(i);
            if (std::string(bName) == "Unknown Biome" && i > 13)
              continue;
            if (ImGui::Selectable(bName, false)) {
              a.biomePreferences[i] = 0.5f;
            }
          }
          ImGui::EndCombo();
        }

        ImGui::Separator();

        // --- PRODUCTION TABLES ---
        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f),
                           "Resource Production (Living)");
        if (ImGui::BeginTable("LivingProd", 3, ImGuiTableFlags_Borders)) {
          ImGui::TableSetupColumn("Resource");
          ImGui::TableSetupColumn("Rate (per tick)");
          ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed,
                                  40);
          ImGui::TableHeadersRow();

          for (auto it = a.livingOutput.begin(); it != a.livingOutput.end();) {
            ImGui::PushID(it->first);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("%s",
                        AssetManager::resourceRegistry[it->first].name.c_str());
            ImGui::TableNextColumn();
            ImGui::SliderFloat("##lprod", &it->second, 0.0f, 10.0f);
            ImGui::TableNextColumn();
            if (ImGui::Button("X##dell")) {
              it = a.livingOutput.erase(it);
              ImGui::PopID();
              continue;
            }
            ImGui::PopID();
            ++it;
          }
          ImGui::EndTable();
        }
        if (ImGui::BeginCombo("##addLiving", "Add Living Output...")) {
          for (int i = 0; i < (int)AssetManager::resourceRegistry.size(); ++i) {
            if (ImGui::Selectable(
                    AssetManager::resourceRegistry[i].name.c_str()))
              a.livingOutput[i] = 1.0f;
          }
          ImGui::EndCombo();
        }

        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f),
                           "Resource Production (Harvested)");
        if (ImGui::BeginTable("HarvestProd", 3, ImGuiTableFlags_Borders)) {
          ImGui::TableSetupColumn("Resource");
          ImGui::TableSetupColumn("Amount");
          ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed,
                                  40);
          ImGui::TableHeadersRow();

          for (auto it = a.harvestOutput.begin();
               it != a.harvestOutput.end();) {
            ImGui::PushID(it->first);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("%s",
                        AssetManager::resourceRegistry[it->first].name.c_str());
            ImGui::TableNextColumn();
            ImGui::SliderFloat("##hprod", &it->second, 0.0f, 100.0f);
            ImGui::TableNextColumn();
            if (ImGui::Button("X##delh")) {
              it = a.harvestOutput.erase(it);
              ImGui::PopID();
              continue;
            }
            ImGui::PopID();
            ++it;
          }
          ImGui::EndTable();
        }
        if (ImGui::BeginCombo("##addHarvest", "Add Harvest Output...")) {
          for (int i = 0; i < (int)AssetManager::resourceRegistry.size(); ++i) {
            if (ImGui::Selectable(
                    AssetManager::resourceRegistry[i].name.c_str()))
              a.harvestOutput[i] = 1.0f;
          }
          ImGui::EndCombo();
        }
      }
    }

    if (a.isResource) {
      if (ImGui::CollapsingHeader("Resource Properties",
                                  ImGuiTreeNodeFlags_DefaultOpen)) {
        // --- SCARCITY ---
        const char *scarcityLabels[] = {"Mythic", "Very Rare", "Rare", "Common",
                                        "Abundant"};
        int sTier = (int)(a.scarcity * 4.0f);
        if (ImGui::Combo("Scarcity Level", &sTier, scarcityLabels, 5))
          a.scarcity = sTier / 4.0f;

        // --- RENEWABILITY ---
        ImGui::Checkbox("Is Renewable", &a.isRenewable);
        ImGui::SameLine();
        ImGui::TextDisabled("(?) Regrows/Reproduces vs Finite/Mineral");

        ImGui::Separator();

        // --- SPAWN LOCATIONS (BIOMES) ---
        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f),
                           "Natural Spawn Biomes");
        if (ImGui::BeginTable("SpawnBiomes", 2, ImGuiTableFlags_Borders)) {
          ImGui::TableSetupColumn("Biome ID");
          ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed,
                                  40);
          ImGui::TableHeadersRow();

          for (size_t i = 0; i < a.spawnBiomeIDs.size(); ++i) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("%s (%d)", GetBiomeName(a.spawnBiomeIDs[i]),
                        a.spawnBiomeIDs[i]);
            ImGui::TableNextColumn();
            if (ImGui::Button(("X##dsb" + std::to_string(i)).c_str())) {
              a.spawnBiomeIDs.erase(a.spawnBiomeIDs.begin() + i);
              break;
            }
          }
          ImGui::EndTable();
        }

        if (ImGui::BeginCombo("##addSpawn", "Add Spawn Biome...")) {
          for (int i = 0; i < 25; ++i) {
            const char *bName = GetBiomeName(i);
            if (std::string(bName) == "Unknown Biome" && i > 13)
              continue;
            bool alreadyIn = false;
            for (int sid : a.spawnBiomeIDs)
              if (sid == i)
                alreadyIn = true;

            if (!alreadyIn && ImGui::Selectable(bName)) {
              a.spawnBiomeIDs.push_back(i);
            }
          }
          ImGui::EndCombo();
        }
      }
    }

    if (a.isBiome) {
      if (ImGui::CollapsingHeader("Biome Settings")) {
        float tModLabel = a.biomeTempMod * 100.0f; // Scale to degrees delta
        if (useFahrenheit)
          tModLabel *= 1.8f;
        ImGui::SliderFloat(useFahrenheit ? "Temp Mod (°F)" : "Temp Mod (°C)",
                           &a.biomeTempMod, -1.0f, 1.0f, "%.2f");
        ImGui::TextDisabled("Adds %.1f %s to local climate", tModLabel,
                            useFahrenheit ? "°F" : "°C");

        ImGui::SliderFloat("Moisture Mod", &a.biomeMoistureMod, -1.0f, 1.0f);
        ImGui::ColorEdit4("Map Overlay Color", (float *)&a.biomeColor);
      }
    }

    if (a.isFaction) {
      if (ImGui::CollapsingHeader("Faction & Culture")) {
        ImGui::InputInt("Formation Year", &a.formationYear);
        ImGui::InputInt("Formation Cell ID", &a.formationCell);

        // --- NEW: LINK TO AGENT (FOUNDER) ---
        // Find existing agent ID if possible, mostly relies on WikiArticle ID
        // vs Agent ID For now, we store just the ID in a custom field or reuse
        // a specific one. Let's add a "founderSpecies" field to WikiArticle? Or
        // just repurpose data map for now to show the UI concept.

        static int selectedAgentIdx = -1;
        std::string currentAgentName =
            a.data.count("Founder") ? a.data["Founder"] : "None";

        if (ImGui::BeginCombo("Founding Species", currentAgentName.c_str())) {
          // Search wikiDB for agents
          for (const auto &cand : wikiDB) {
            if (cand.isAgent) {
              if (ImGui::Selectable(cand.title.c_str())) {
                a.data["Founder"] = cand.title;
                // Ideally store ID: a.data["FounderID"] =
                // std::to_string(cand.id);
              }
            }
          }
          ImGui::EndCombo();
        }

        ImGui::Separator();
        ImGui::Text("Historical Events");
        if (ImGui::BeginTable("FactionEvents", 2, ImGuiTableFlags_Borders)) {
          ImGui::TableSetupColumn("Event ID");
          ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed,
                                  40);
          for (int i = 0; i < (int)a.linkedEventIDs.size(); ++i) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Event #%d", a.linkedEventIDs[i]);
            ImGui::TableNextColumn();
            if (ImGui::Button(("X##ev" + std::to_string(i)).c_str())) {
              a.linkedEventIDs.erase(a.linkedEventIDs.begin() + i);
            }
          }
          ImGui::EndTable();
        }
        static int newEvID = 0;
        ImGui::SetNextItemWidth(100);
        ImGui::InputInt("##newEv", &newEvID);
        ImGui::SameLine();
        if (ImGui::Button("Add Event Link")) {
          a.linkedEventIDs.push_back(newEvID);
        }
      }
    }

    ImGui::Separator();
    if (ImGui::CollapsingHeader("Map Attachment")) {
      ImGui::Checkbox("Has Location", &a.hasLocation);
      if (a.hasLocation) {
        ImGui::DragInt("Map X", &a.mapX, 1.0f, 0, 1000);
        ImGui::DragInt("Map Y", &a.mapY, 1.0f, 0, 1000);
        if (ImGui::Button("Center on Map")) {
          // Logic to jump camera could go here in a unified app
        }
      }
    }

    ImGui::Separator();
    CategoryTemplate *tmpl = nullptr;
    for (auto &t : templates)
      if (t.name == a.categoryName)
        tmpl = &t;

    if (tmpl) {
      if (ImGui::BeginTable("Form", 2)) {
        for (const auto &f : tmpl->fields) {
          ImGui::TableNextRow();
          ImGui::TableNextColumn();
          ImGui::Text("%s", f.label.c_str());
          ImGui::TableNextColumn();

          std::string &val = a.data[f.label];

          ImGui::PushID(f.label.c_str());
          if (f.type == FieldType::TEXT) {
            ImGui::InputText("##v", &val);
          } else if (f.type == FieldType::NUMBER) {
            float v = 0.0f;
            try {
              if (!val.empty())
                v = std::stof(val);
            } catch (...) {
            }
            if (ImGui::DragFloat("##v", &v))
              val = std::to_string(v);
          } else if (f.type == FieldType::SLIDER) {
            float v = 0.5f;
            try {
              if (!val.empty())
                v = std::stof(val);
            } catch (...) {
            }
            if (ImGui::SliderFloat("##v", &v, 0.0f, 1.0f))
              val = std::to_string(v);
          } else if (f.type == FieldType::IMAGE_PATH ||
                     f.type == FieldType::SPRITE_REF) {
            ImGui::InputText("##v", &val);
            if (f.type == FieldType::SPRITE_REF) {
              if (ImGui::IsItemHovered())
                ImGui::SetTooltip(
                    "This image will be used in Replay Viewer for this Agent.");
            }
          } else if (f.type == FieldType::COLOR) {
            ImVec4 col(1, 1, 1, 1);
            if (ImGui::ColorEdit4("##v", (float *)&col)) {
              val = std::to_string(col.x) + "," + std::to_string(col.y) + "," +
                    std::to_string(col.z) + "," + std::to_string(col.w);
            }
          }
          ImGui::PopID();
        }
        ImGui::EndTable();
      }
    }

    ImGui::Separator();
    ImGui::Text("Tags:");
    ImGui::SameLine();
    static std::string newTag;
    ImGui::SetNextItemWidth(100);
    ImGui::InputText("##nt", &newTag);
    ImGui::SameLine();
    if (ImGui::Button("+") && !newTag.empty()) {
      a.tags.push_back(newTag);
      newTag = "";
    }

    for (auto &t : a.tags) {
      ImGui::SameLine();
      ImGui::Button(t.c_str());
    }

    ImGui::Separator();
    if (a.imageTexture) {
      ImGui::Image((void *)(intptr_t)a.imageTexture, ImVec2(200, 200));
      if (ImGui::Button("Reload Image"))
        a.imageTexture = LoadTexture(a.imagePath);
    } else {
      ImGui::InputText("Image Path", &a.imagePath);
      if (ImGui::Button("Load Image") && !a.imagePath.empty())
        a.imageTexture = LoadTexture(a.imagePath);
    }

    ImGui::Separator();
    ImGui::Text("Lore (Markdown Supported):");

    if (ImGui::BeginTabBar("ContentTabs")) {
      if (ImGui::BeginTabItem("Edit")) {
        ImGui::InputTextMultiline("##c", &a.content, ImVec2(-1, -1));
        ImGui::EndTabItem();
      }
      if (ImGui::BeginTabItem("Preview")) {
        RenderMarkdown(a.content);
        ImGui::EndTabItem();
      }
      ImGui::EndTabBar();
    }

  } else {
    ImGui::TextDisabled("Select an article to edit.");
  }

  ImGui::End();
}

void DrawCalendarEditor() {
  if (!ImGui::Begin("Timekeeper")) {
    ImGui::End();
    return;
  }

  ImGui::TextColored(ImVec4(1, 0.8f, 0.2f, 1), "%s Settings",
                     calendar.name.c_str());
  ImGui::InputText("Calendar Name", &calendar.name);

  if (ImGui::BeginTabBar("CalTabs")) {

    // --- TAB 1: MONTHS & DAYS ---
    if (ImGui::BeginTabItem("Structure")) {
      ImGui::Separator();
      ImGui::Text("Months");
      ImGui::BeginChild("Months", ImVec2(0, 200), true);
      for (int i = 0; i < (int)calendar.monthNames.size(); ++i) {
        ImGui::PushID(i);
        ImGui::SetNextItemWidth(30);
        ImGui::Text("%2d", i + 1);
        ImGui::SameLine();

        ImGui::SetNextItemWidth(150);
        ImGui::InputText("##name", &calendar.monthNames[i]);
        ImGui::SameLine();

        ImGui::SetNextItemWidth(80);
        ImGui::InputInt("Days", &calendar.monthLengths[i]);
        ImGui::SameLine();

        if (ImGui::Button("X")) {
          calendar.monthNames.erase(calendar.monthNames.begin() + i);
          calendar.monthLengths.erase(calendar.monthLengths.begin() + i);
        }
        ImGui::PopID();
      }
      if (ImGui::Button("+ Add Month")) {
        calendar.monthNames.push_back("New Month");
        calendar.monthLengths.push_back(30);
      }
      ImGui::EndChild();

      ImGui::Separator();
      ImGui::Text("Weekdays");
      for (int i = 0; i < (int)calendar.weekDayNames.size(); ++i) {
        ImGui::PushID(i + 1000);
        ImGui::SetNextItemWidth(150);
        ImGui::InputText("##wd", &calendar.weekDayNames[i]);
        ImGui::SameLine();
        if (ImGui::Button("X")) {
          calendar.weekDayNames.erase(calendar.weekDayNames.begin() + i);
        }
        ImGui::PopID();
      }
      if (ImGui::Button("+ Add Weekday"))
        calendar.weekDayNames.push_back("New Day");

      ImGui::EndTabItem();
    }

    // --- TAB 2: CELESTIAL BODIES (MOONS) ---
    if (ImGui::BeginTabItem("Moons")) {
      ImGui::TextWrapped("Moons affect night visibility and lycanthropy events "
                         "in the simulation.");
      for (int i = 0; i < (int)calendar.moons.size(); ++i) {
        Moon &m = calendar.moons[i];
        ImGui::PushID(i + 2000);
        if (ImGui::CollapsingHeader(m.name.c_str(),
                                    ImGuiTreeNodeFlags_DefaultOpen)) {
          ImGui::InputText("Name", &m.name);
          ImGui::DragFloat("Cycle (Days)", &m.cycle, 0.1f, 1.0f, 1000.0f);
          ImGui::SliderFloat("Phase Offset", &m.shift, 0.0f, m.cycle);
          ImGui::ColorEdit3("Moon Color", (float *)&m.color);

          ImGui::Text("Preview:");
          ImGui::SameLine();
          ImGui::ColorButton("##preview", m.color, 0, ImVec2(30, 30));

          if (ImGui::Button("Delete Moon")) {
            calendar.moons.erase(calendar.moons.begin() + i);
            ImGui::PopID();
            break;
          }
        }
        ImGui::PopID();
      }
      if (ImGui::Button("Add Moon"))
        calendar.moons.push_back({"New Moon"});
      ImGui::EndTabItem();
    }

    // --- TAB 3: SEASONS ---
    if (ImGui::BeginTabItem("Seasons")) {
      ImGui::TextWrapped("Seasons drive the Climate Engine.");
      for (int i = 0; i < (int)calendar.seasons.size(); ++i) {
        Season &s = calendar.seasons[i];
        ImGui::PushID(i + 3000);
        ImGui::InputText("##sn", &s.name);
        ImGui::SameLine();
        ImGui::Text("Starts Month:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(60);
        ImGui::InputInt("##sm", &s.startMonth);
        ImGui::SameLine();
        if (ImGui::Button("X")) {
          calendar.seasons.erase(calendar.seasons.begin() + i);
          ImGui::PopID();
          break;
        }
        ImGui::PopID();
      }
      if (ImGui::Button("Add Season"))
        calendar.seasons.push_back({"New Season"});
      ImGui::EndTabItem();
    }

    ImGui::EndTabBar();
  }

  ImGui::Separator();
  if (ImGui::Button("SAVE CALENDAR CONFIG", ImVec2(-1, 40)))
    calendar.Save();

  ImGui::End();
}

void DrawHelp() {
  if (!showHelp)
    return;

  static bool contentLoaded = false;
  static std::string helpContent;
  if (!contentLoaded) {
    std::ifstream f(SagaConfig::MANUAL_MD);
    if (f.is_open()) {
      std::stringstream ss;
      ss << f.rdbuf();
      helpContent = ss.str();
    } else {
      helpContent =
          "# Help File Not Found\n\nPlease ensure `data/manual.md` exists.";
    }
    contentLoaded = true;
  }

  ImGui::SetNextWindowSize(ImVec2(600, 500), ImGuiCond_FirstUseEver);
  ImGui::OpenPopup("Help Modal");
  if (ImGui::BeginPopupModal("Help Modal", &showHelp)) {
    ImGui::TextColored(ImVec4(1, 0.8f, 0.2f, 1), "S.A.G.A. Database Manual");
    ImGui::Separator();

    ImGui::BeginChild("HelpScroll",
                      ImVec2(0, -ImGui::GetFrameHeightWithSpacing()));
    RenderMarkdown(helpContent);
    ImGui::EndChild();

    ImGui::Separator();
    if (ImGui::Button("Close", ImVec2(120, 0))) {
      showHelp = false;
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
  AssetManager::SyncWithLore();
}

void DrawTimeline() {
  ImGui::Begin("World Timeline", nullptr);

  // Scannable horizontal timeline
  ImGui::BeginChild("TimeScroll", ImVec2(0, 80), true,
                    ImGuiWindowFlags_HorizontalScrollbar);

  ImDrawList *drawList = ImGui::GetWindowDrawList();
  ImVec2 startPos = ImGui::GetCursorScreenPos();
  float pixelPerYear = 20.0f;
  float timelineHeight = 40.0f;

  // Draw Axis
  drawList->AddLine(
      ImVec2(startPos.x, startPos.y + timelineHeight),
      ImVec2(startPos.x + 500 * pixelPerYear, startPos.y + timelineHeight),
      IM_COL32(200, 200, 200, 255), 2.0f);

  // Mark Years and Events
  for (int year = 0; year <= 500; year += 10) {
    ImVec2 p =
        ImVec2(startPos.x + (year * pixelPerYear), startPos.y + timelineHeight);
    drawList->AddLine(p, ImVec2(p.x, p.y + 10), IM_COL32(200, 200, 200, 255),
                      1.0f);
    if (year % 50 == 0) {
      char buf[16];
      sprintf(buf, "%d", year);
      drawList->AddText(ImVec2(p.x - 5, p.y + 12), IM_COL32(255, 255, 255, 255),
                        buf);
    }
  }

  // Highlight events from wikiDB if they have a 'Year' field
  for (int i = 0; i < (int)wikiDB.size(); ++i) {
    auto &a = wikiDB[i];
    if (a.data.count("Year")) {
      try {
        int yr = std::stoi(a.data.at("Year"));
        ImVec2 ep = ImVec2(startPos.x + (yr * pixelPerYear),
                           startPos.y + timelineHeight);

        ImU32 col = IM_COL32(255, 215, 0, 255); // Gold
        drawList->AddCircleFilled(ep, 6.0f, col);
        drawList->AddCircle(ep, 8.0f, col, 12, 1.0f);

        if (ImGui::IsMouseHoveringRect(ImVec2(ep.x - 10, ep.y - 10),
                                       ImVec2(ep.x + 10, ep.y + 10))) {
          ImGui::SetTooltip("%s (Year %d)\nClick to View", a.title.c_str(), yr);
          if (ImGui::IsMouseClicked(0)) {
            selectedIdx = i;
          }
        }
      } catch (...) {
      }
    }
  }

  ImGui::EndChild();
  ImGui::End();
}

// --- MAIN ---
int main(int, char **) {
  if (!glfwInit())
    return 1;
  GLFWwindow *w = glfwCreateWindow(1600, 900, "S.A.G.A. Database", NULL, NULL);
  if (!w)
    return 1;
  glfwMakeContextCurrent(w);
  glewInit();
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  SetupTheme();
  ImGui_ImplGlfw_InitForOpenGL(w, true);
  ImGui_ImplOpenGL3_Init("#version 130");

  LoadData();

  while (!glfwWindowShouldClose(w)) {
    glfwPollEvents();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    int dw, dh;
    glfwGetFramebufferSize(w, &dw, &dh);

    // Sidebar: 200px
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(250, (float)dh - 150));
    DrawSidebar();

    // Main: Center
    ImGui::SetNextWindowPos(ImVec2(250, 0));
    ImGui::SetNextWindowSize(ImVec2((float)dw - 800, (float)dh - 150));
    DrawMainEditor();

    // Right Column: Template Builder (Top half)
    ImGui::SetNextWindowPos(ImVec2((float)dw - 550, 0));
    ImGui::SetNextWindowSize(ImVec2(550, (float)dh / 2 - 75));
    DrawTemplateEditor();

    // Right Column: Calendar (Bottom half)
    ImGui::SetNextWindowPos(ImVec2((float)dw - 550, (float)dh / 2 - 75));
    ImGui::SetNextWindowSize(ImVec2(550, (float)dh / 2 - 75));
    DrawCalendarEditor();

    // Bottom: Timeline
    ImGui::SetNextWindowPos(ImVec2(0, (float)dh - 150));
    ImGui::SetNextWindowSize(ImVec2((float)dw, 150));
    DrawTimeline();

    DrawHelp();

    ImGui::Render();
    glViewport(0, 0, dw, dh);
    glClearColor(0.1f, 0.1f, 0.1f, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(w);
  }
  SaveData();

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  glfwDestroyWindow(w);
  glfwTerminate();

  return 0;
}
