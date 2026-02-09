#include "../../include/AssetManager.hpp"
#include "../../include/BinaryExporter.hpp"
#include "../../include/PlatformUtils.hpp"
#include "../../include/Theme.hpp"
#include "../../include/WorldEngine.hpp"

#include "../../deps/imgui/backends/imgui_impl_glfw.h"
#include "../../deps/imgui/backends/imgui_impl_opengl3.h"
#include "../../deps/imgui/imgui.h"
#include "../../deps/imgui/misc/cpp/imgui_stdlib.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

// Note: Using manual JSON formatting to match existing AssetManager.cpp pattern
// for maximum stability and speed without external header dependencies.

// --- ENUMS & STRUCTS ---
enum class ArticleType {
  LOCATION,
  FACTION,
  FLORA,
  FAUNA,
  EVENT,
  CHARACTER,
  RESOURCE
};

const char *TypeNames[] = {"Location",       "Faction", "Flora (Plant)",
                           "Fauna (Animal)", "Event",   "Character",
                           "Resource"};

struct WikiArticle {
  int id;
  std::string title;
  ArticleType type;
  std::string content;

  // Geo-Spatial
  bool hasLocation = false;
  int mapX = 0, mapY = 0;

  // Link to Simulation Engine
  int simID = -1;

  // Timeline
  bool isTimelineEvent = false;
  int year = 0, month = 1;
  std::string simEffect;
};

// --- GLOBALS ---
std::vector<WikiArticle> wikiDB;
WorldBuffers mapData;
GLuint mapTexture = 0;
int selectedIdx = -1;
bool showMapOverlay = false;

// --- HELPER: HELPER UI FOR MAPS (DIET/OUTPUT) ---
void DrawResourceMap(const char *label, std::map<int, float> &dataMap) {
  ImGui::PushID(label);
  ImGui::Text("%s", label);
  ImGui::Indent();

  // List existing
  std::vector<int> toRemove;
  for (auto &[resID, amount] : dataMap) {
    std::string name = "Unknown ID:" + std::to_string(resID);
    for (auto &r : AssetManager::resourceRegistry)
      if (r.id == resID)
        name = r.name;

    ImGui::PushID(resID);
    if (ImGui::Button("X"))
      toRemove.push_back(resID);
    ImGui::SameLine();
    ImGui::Text("%s:", name.c_str());
    ImGui::SameLine();
    ImGui::SetNextItemWidth(60);
    ImGui::DragFloat("##amt", &amount, 0.1f, 0.0f, 100.0f);
    ImGui::PopID();
  }
  for (int id : toRemove)
    dataMap.erase(id);

  // Add New
  static int selRes = 0;
  if (!AssetManager::resourceRegistry.empty()) {
    if (selRes >= (int)AssetManager::resourceRegistry.size())
      selRes = 0;
    const char *preview = AssetManager::resourceRegistry[selRes].name.c_str();
    ImGui::SetNextItemWidth(100);
    if (ImGui::BeginCombo("##add", preview)) {
      for (int i = 0; i < (int)AssetManager::resourceRegistry.size(); ++i) {
        if (ImGui::Selectable(AssetManager::resourceRegistry[i].name.c_str(),
                              selRes == i))
          selRes = i;
      }
      ImGui::EndCombo();
    }
    ImGui::SameLine();
    if (ImGui::Button("Add")) {
      dataMap[AssetManager::resourceRegistry[selRes].id] = 1.0f;
    }
  }
  ImGui::Unindent();
  ImGui::PopID();
}

// --- FILE IO (Manual JSON) ---
void SaveAllData() {
  // 1. Save Lore
  std::ofstream o("data/lore.json");
  if (o.is_open()) {
    o << "[\n";
    for (size_t i = 0; i < wikiDB.size(); ++i) {
      const auto &a = wikiDB[i];
      o << "  {\n";
      o << "    \"id\": " << a.id << ",\n";
      o << "    \"title\": \"" << a.title << "\",\n";
      o << "    \"type\": " << (int)a.type << ",\n";
      o << "    \"content\": \"" << a.content << "\",\n";
      o << "    \"simID\": " << a.simID;
      if (a.hasLocation) {
        o << ",\n    \"loc\": [" << a.mapX << ", " << a.mapY << "]";
      }
      if (a.isTimelineEvent) {
        o << ",\n    \"time\": [" << a.year << ", " << a.month << "],\n";
        o << "    \"effect\": \"" << a.simEffect << "\"";
      }
      o << "\n  }";
      if (i < wikiDB.size() - 1)
        o << ",";
      o << "\n";
    }
    o << "]\n";
    o.close();
  }

  // 2. Save Rules
  AssetManager::SaveAll();
}

void LoadAllData() {
  AssetManager::LoadAll(); // Load Rules first

  // Note: Manual loader is complex, for now we re-initialize wikiDB if file is
  // missing or use a simplified mock loader as this is a transition step.
}

// --- MAP VISUALS ---
void GenerateMapTexture() {
  if (!mapData.height)
    return;
  int w = 1000;
  int h = 1000;
  std::vector<unsigned char> pixels(w * h * 3);
  for (int i = 0; i < w * h; ++i) {
    float hgt = mapData.height[i];
    unsigned char r, g, b;
    if (hgt < 0.4f) {
      r = 10;
      g = 40;
      b = 120;
    } else if (hgt < 0.45f) {
      r = 200;
      g = 190;
      b = 100;
    } else {
      r = 30;
      g = 100;
      b = 30;
    }
    pixels[i * 3] = r;
    pixels[i * 3 + 1] = g;
    pixels[i * 3 + 2] = b;
  }
  glGenTextures(1, &mapTexture);
  glBindTexture(GL_TEXTURE_2D, mapTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE,
               pixels.data());
}

// --- UI LOGIC ---

void DrawSidebar() {
  ImGui::Begin("S.A.G.A. Database", nullptr, ImGuiWindowFlags_NoCollapse);

  // NEW ARTICLE BUTTON
  if (ImGui::Button("+ New Article", ImVec2(-1, 40))) {
    WikiArticle a;
    a.id = rand();
    a.title = "New Entry";
    a.type = ArticleType::LOCATION;
    wikiDB.push_back(a);
    selectedIdx = wikiDB.size() - 1;
  }

  ImGui::Separator();
  ImGui::BeginChild("List");
  for (int i = 0; i < (int)wikiDB.size(); ++i) {
    // Color code types
    ImVec4 color = ImVec4(1, 1, 1, 1);
    if (wikiDB[i].type == ArticleType::FACTION)
      color = ImVec4(1, 0.5f, 0.5f, 1);
    if (wikiDB[i].type == ArticleType::FAUNA)
      color = ImVec4(0.5f, 1, 0.5f, 1);

    ImGui::PushStyleColor(ImGuiCol_Text, color);
    if (ImGui::Selectable(wikiDB[i].title.c_str(), selectedIdx == i))
      selectedIdx = i;
    ImGui::PopStyleColor();
  }
  ImGui::EndChild();
  ImGui::End();
}

void DrawMainEditor() {
  ImGui::Begin("Inspector");

  if (selectedIdx >= 0 && selectedIdx < (int)wikiDB.size()) {
    WikiArticle &a = wikiDB[selectedIdx];

    // --- HEADER ---
    ImGui::InputText("Title", &a.title);

    // Type Selector
    int typeInt = (int)a.type;
    if (ImGui::Combo("Category", &typeInt, TypeNames, 7)) {
      a.type = (ArticleType)typeInt;
    }

    // --- MAP LOCATION ---
    ImGui::SameLine();
    if (ImGui::Button(a.hasLocation ? "Edit Location" : "Mark on Map")) {
      showMapOverlay = true;
      if (!mapData.height) { // Lazy load map
        mapData.Initialize(1000000);
        if (BinaryExporter::LoadWorld(mapData, "data/world.map"))
          GenerateMapTexture();
      }
    }
    if (a.hasLocation)
      ImGui::TextDisabled("Coords: %d, %d", a.mapX, a.mapY);

    ImGui::Separator();

    // --- DYNAMIC SIMULATION PROPERTIES ---
    // Links Lore to Code

    if (a.type == ArticleType::FACTION || a.type == ArticleType::FLORA ||
        a.type == ArticleType::FAUNA) {

      // Check if linked
      if (a.simID == -1) {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "No Simulation Rules Found.");
        if (ImGui::Button("Create Sim Agent")) {
          AssetManager::CreateNewAgent();
          a.simID = AssetManager::agentRegistry.back().id;
          AssetManager::agentRegistry.back().name = a.title; // Sync name

          if (a.type == ArticleType::FACTION)
            AssetManager::agentRegistry.back().type = AgentType::CIVILIZED;
          if (a.type == ArticleType::FAUNA)
            AssetManager::agentRegistry.back().type = AgentType::FAUNA;
          if (a.type == ArticleType::FLORA)
            AssetManager::agentRegistry.back().type = AgentType::FLORA;
        }
      } else {
        AgentDefinition *agent = nullptr;
        for (auto &ag : AssetManager::agentRegistry)
          if (ag.id == a.simID)
            agent = &ag;

        if (agent) {
          ImGui::TextColored(ImVec4(0, 1, 1, 1),
                             "Simulation Properties (Sim ID: %d)", a.simID);
          ImGui::Indent();

          if (agent->name != a.title) {
            if (ImGui::Button("Sync Name to Agent"))
              agent->name = a.title;
          }

          ImGui::ColorEdit3("Map Color", agent->color);
          ImGui::SliderFloat("Ideal Temp", &agent->idealTemp, 0.0f, 1.0f,
                             "Cold %.2f Hot");
          ImGui::SliderFloat("Resilience", &agent->resilience, 0.0f, 1.0f);
          ImGui::SliderFloat("Spread/Move Speed", &agent->expansionRate, 0.0f,
                             2.0f);

          if (a.type == ArticleType::FACTION) {
            ImGui::SliderFloat("Aggression", &agent->aggression, 0.0f, 1.0f);
          }

          DrawResourceMap("Diet / Upkeep:", agent->diet);
          DrawResourceMap("Produces / Yield:", agent->output);

          ImGui::Unindent();
        }
      }
    } else if (a.type == ArticleType::RESOURCE) {
      if (a.simID == -1) {
        if (ImGui::Button("Create Resource Definition")) {
          AssetManager::CreateNewResource();
          a.simID = AssetManager::resourceRegistry.back().id;
          AssetManager::resourceRegistry.back().name = a.title;
        }
      } else {
        ResourceDef *res = nullptr;
        for (auto &r : AssetManager::resourceRegistry)
          if (r.id == a.simID)
            res = &r;

        if (res) {
          ImGui::TextColored(ImVec4(1, 1, 0, 1), "Resource Stats (ID: %d)",
                             a.simID);
          ImGui::InputFloat("Market Value", &res->value);
          ImGui::Checkbox("Renewable?", &res->isRenewable);
        }
      }
    } else if (a.type == ArticleType::EVENT) {
      a.isTimelineEvent = true;
      ImGui::Separator();
      ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "Timeline Trigger");
      ImGui::InputInt("Year", &a.year);
      const char *effects[] = {"NONE", "TRIGGER_ICE_AGE",
                               "TRIGGER_GLOBAL_WARMING", "SPAWN_PLAGUE",
                               "BOOST_MAGIC"};
      if (ImGui::BeginCombo("Effect", a.simEffect.c_str())) {
        for (auto e : effects) {
          if (ImGui::Selectable(e, a.simEffect == e))
            a.simEffect = e;
        }
        ImGui::EndCombo();
      }
    }

    ImGui::Separator();

    ImGui::Text("Lore / Description:");
    ImGui::InputTextMultiline("##content", &a.content, ImVec2(-1, -1));

  } else {
    ImGui::TextDisabled("Select an article to edit.");
  }

  // GLOBAL SAVE BUTTON
  ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() - 120, 30));
  if (ImGui::Button("SAVE ALL", ImVec2(100, 30))) {
    SaveAllData();
  }

  ImGui::End();
}

void DrawMapModal() {
  if (!showMapOverlay)
    return;
  ImGui::OpenPopup("Select Location");

  if (ImGui::BeginPopupModal("Select Location", &showMapOverlay,
                             ImGuiWindowFlags_AlwaysAutoResize)) {
    if (mapTexture) {
      ImGui::Image((void *)(intptr_t)mapTexture, ImVec2(600, 600));
    } else {
      ImGui::Text("No World Map found in data/world.map");
    }

    if (ImGui::Button("Close")) {
      showMapOverlay = false;
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
}

// --- MAIN ---
int main(int, char **) {
  if (!glfwInit())
    return 1;
  GLFWwindow *w =
      glfwCreateWindow(1400, 800, "S.A.G.A. Unified Database", NULL, NULL);
  glfwMakeContextCurrent(w);
  glewInit();
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  SetupSAGATheme();
  ImGui_ImplGlfw_InitForOpenGL(w, true);
  ImGui_ImplOpenGL3_Init("#version 130");

  LoadAllData();

  while (!glfwWindowShouldClose(w)) {
    glfwPollEvents();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Manual Simple Split Layout (to avoid DockSpaceOverViewport build issues)
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("MasterWindow", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoBringToFrontOnFocus);

    DrawSidebar();
    ImGui::SameLine();
    DrawMainEditor();
    DrawMapModal();

    ImGui::End();

    ImGui::Render();
    int dw, dh;
    glfwGetFramebufferSize(w, &dw, &dh);
    glViewport(0, 0, dw, dh);
    glClearColor(0.1f, 0.1f, 0.1f, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(w);
  }
  return 0;
}
