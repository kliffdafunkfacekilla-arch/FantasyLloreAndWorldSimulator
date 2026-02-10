#include "../../include/AssetManager.hpp"
#include "../../include/BinaryExporter.hpp"
#include "../../include/WorldEngine.hpp"
#include "../../include/nlohmann/json.hpp"
#include "../../include/stb_image.h"

#include "../../deps/imgui/backends/imgui_impl_glfw.h"
#include "../../deps/imgui/backends/imgui_impl_opengl3.h"
#include "../../deps/imgui/imgui.h"
#include "../../deps/imgui/misc/cpp/imgui_stdlib.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

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

    std::ofstream o("data/calendar.json");
    o << std::setw(4) << j;
  }

  void Load() {
    std::ifstream f("data/calendar.json");
    if (f.is_open()) {
      try {
        nlohmann::json j;
        f >> j;
        name = j.value("name", "New Calendar");
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
      } catch (...) {
      }
    }
  }
};

// --- DATA STRUCTURES ---
enum class FieldType { TEXT, NUMBER, SLIDER, COLOR, IMAGE_PATH, SPRITE_REF };

struct CategoryTemplate {
  std::string name;
  struct FieldDef {
    std::string label;
    FieldType type;
  };
  std::vector<FieldDef> fields;
};

struct WikiArticle {
  int id;
  std::string title;
  std::string categoryName; // Links to CategoryTemplate
  std::string content;      // Main Body (Markdown)

  // Dynamic Data (Matches Template)
  std::map<std::string, std::string> data;

  // Links
  std::vector<std::string> tags;
  int simID = -1; // Links to AgentSystem
  bool hasLocation = false;
  int mapX = 0, mapY = 0;

  // Media
  std::string imagePath;   // Main illustration
  GLuint imageTexture = 0; // GPU Handle
};

// --- GLOBALS ---
CalendarSystem calendar;
std::vector<WikiArticle> wikiDB;
std::vector<CategoryTemplate> templates;
WorldBuffers mapData;
GLuint mapTexture = 0;
int selectedIdx = -1;
bool showMapOverlay = false;
bool showHelp = false;

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
  nlohmann::json root = nlohmann::json::array();
  for (const auto &a : wikiDB) {
    nlohmann::json obj;
    obj["id"] = a.id;
    obj["title"] = a.title;
    obj["cat"] = a.categoryName;
    obj["content"] = a.content;
    obj["data"] = a.data;
    obj["tags"] = a.tags;
    obj["simID"] = a.simID;
    obj["img"] = a.imagePath;
    if (a.hasLocation)
      obj["loc"] = {a.mapX, a.mapY};
    root.push_back(obj);
  }
  std::ofstream o("data/lore.json");
  o << std::setw(4) << root;

  // Save Templates
  nlohmann::json tRoot = nlohmann::json::array();
  for (const auto &t : templates) {
    nlohmann::json tObj;
    tObj["name"] = t.name;
    nlohmann::json fields = nlohmann::json::array();
    for (const auto &f : t.fields)
      fields.push_back({{"l", f.label}, {"t", (int)f.type}});
    tObj["fields"] = fields;
    tRoot.push_back(tObj);
  }
  std::ofstream ot("data/templates.json");
  ot << std::setw(4) << tRoot;

  AssetManager::SaveAll(); // Save Sim Rules
  calendar.Save();
}

void LoadData() {
  AssetManager::LoadAll();
  calendar.Load();

  // Load Templates
  std::ifstream ft("data/templates.json");
  if (ft.is_open()) {
    try {
      nlohmann::json tRoot;
      ft >> tRoot;
      templates.clear();
      for (const auto &tObj : tRoot) {
        CategoryTemplate t;
        t.name = tObj["name"];
        for (const auto &fObj : tObj["fields"])
          t.fields.push_back({fObj["l"], (FieldType)fObj["t"]});
        templates.push_back(t);
      }
    } catch (...) {
    }
  }
  if (templates.empty()) {
    templates.push_back({"General", {{"Description", FieldType::TEXT}}});
    templates.push_back(
        {"Faction",
         {{"Population", FieldType::NUMBER}, {"Color", FieldType::COLOR}}});
  }

  // Load Wiki
  std::ifstream f("data/lore.json");
  if (!f.is_open())
    return;
  try {
    nlohmann::json root;
    f >> root;
    wikiDB.clear();
    for (auto &obj : root) {
      WikiArticle a;
      a.id = obj.value("id", 0);
      a.title = obj.value("title", "Untitled");
      a.categoryName = obj.value("cat", "General");
      a.content = obj.value("content", "");
      if (obj.contains("data"))
        a.data = obj["data"].get<std::map<std::string, std::string>>();
      if (obj.contains("tags"))
        a.tags = obj["tags"].get<std::vector<std::string>>();
      a.simID = obj.value("simID", -1);
      a.imagePath = obj.value("img", "");
      if (!a.imagePath.empty())
        a.imageTexture = LoadTexture(a.imagePath);

      if (obj.contains("loc")) {
        a.hasLocation = true;
        a.mapX = obj["loc"][0];
        a.mapY = obj["loc"][1];
      }
      wikiDB.push_back(a);
    }
  } catch (...) {
  }
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

  if (ImGui::Button("Import Obsidian Notes", ImVec2(-1, 0))) {
    // Call the external importer script
    // Note: Using absolute path to ensure it's found regardless of process CWD
    system("python "
           "C:/Users/krazy/Documents/GitHub/oracle/BRQSE/scripts/"
           "obsidian_importer.py");
    LoadData(); // Refresh the list
  }
  ImGui::Separator();

  // Filter
  static char filter[64] = "";
  ImGui::InputText("Search", filter, 64);

  ImGui::BeginChild("List");
  for (int i = 0; i < wikiDB.size(); ++i) {
    if (strlen(filter) > 0 && wikiDB[i].title.find(filter) == std::string::npos)
      continue;

    ImVec4 col = ImVec4(1, 1, 1, 1);
    if (wikiDB[i].categoryName == "Faction")
      col = ImVec4(1, 0.6f, 0.6f, 1);

    ImGui::PushStyleColor(ImGuiCol_Text, col);
    if (ImGui::Selectable(wikiDB[i].title.c_str(), selectedIdx == i))
      selectedIdx = i;
    ImGui::PopStyleColor();
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
    if (ImGui::Button("?"))
      showHelp = true;

    if (ImGui::BeginCombo("Category", a.categoryName.c_str())) {
      for (const auto &t : templates) {
        if (ImGui::Selectable(t.name.c_str(), t.name == a.categoryName))
          a.categoryName = t.name;
      }
      ImGui::EndCombo();
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
  ImGui::OpenPopup("Help Modal");
  if (ImGui::BeginPopupModal("Help Modal", &showHelp)) {
    ImGui::Text("S.A.G.A. Wiki Help");
    ImGui::Separator();
    ImGui::Text("MARKDOWN:");
    ImGui::BulletText("# Title (Big Header)");
    ImGui::BulletText("## Subtitle (Small Header)");
    ImGui::BulletText("* List Item");
    ImGui::BulletText("--- (Horizontal Line)");
    ImGui::Separator();
    ImGui::Text("VARIABLES:");
    ImGui::TextWrapped(
        "Use the 'Category Builder' window to define custom stat blocks (e.g. "
        "Health, Mana) for different types of articles.");
    ImGui::Separator();
    if (ImGui::Button("Close")) {
      showHelp = false;
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
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
    ImGui::SetNextWindowSize(ImVec2(250, (float)dh));
    DrawSidebar();

    // Main: Center
    ImGui::SetNextWindowPos(ImVec2(250, 0));
    ImGui::SetNextWindowSize(ImVec2((float)dw - 800, (float)dh));
    DrawMainEditor();

    // Right Column: Template Builder (Top half)
    ImGui::SetNextWindowPos(ImVec2((float)dw - 550, 0));
    ImGui::SetNextWindowSize(ImVec2(550, (float)dh / 2));
    DrawTemplateEditor();

    // Right Column: Calendar (Bottom half)
    ImGui::SetNextWindowPos(ImVec2((float)dw - 550, (float)dh / 2));
    ImGui::SetNextWindowSize(ImVec2(550, (float)dh / 2));
    DrawCalendarEditor();

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
