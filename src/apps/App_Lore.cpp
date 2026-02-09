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
}

void LoadData() {
  AssetManager::LoadAll();
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
  idx << "<h1>S.A.G.A. World Codex</h1><hr>";

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

    // Use a simple non-docking manual layout
    int dw, dh;
    glfwGetFramebufferSize(w, &dw, &dh);

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(300, (float)dh));
    DrawSidebar();

    ImGui::SetNextWindowPos(ImVec2(300, 0));
    ImGui::SetNextWindowSize(ImVec2((float)dw - 600, (float)dh));
    DrawMainEditor();

    ImGui::SetNextWindowPos(ImVec2((float)dw - 300, 0));
    ImGui::SetNextWindowSize(ImVec2(300, (float)dh));
    DrawTemplateEditor();

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
