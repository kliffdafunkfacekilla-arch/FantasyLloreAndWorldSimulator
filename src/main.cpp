#define GLEW_STATIC
#define GLFW_INCLUDE_NONE
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cmath>
#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// 1. Include Your Headers
#include "AssetManager.hpp"
#include "PlatformUtils.hpp"
#include "SimulationModules.hpp"
#include "WorldEngine.hpp"

// Include your Visuals
#include "visuals/MapRenderer.cpp"

// --- Helper: Shader Loader ---
std::string ReadFile(const char *path) {
  std::ifstream file(path);
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

void checkShader(GLuint shader, const std::string &type) {
  GLint success;
  GLchar infoLog[1024];
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(shader, 1024, NULL, infoLog);
    std::cout << "[ERROR] " << type << " Compilation Error:\n"
              << infoLog << "\n";
  }
}

void checkProgram(GLuint program) {
  GLint success;
  GLchar infoLog[1024];
  glGetProgramiv(program, GL_LINK_STATUS, &success);
  if (!success) {
    glGetProgramInfoLog(program, 1024, NULL, infoLog);
    std::cout << "[ERROR] Shader Program Link Error:\n" << infoLog << "\n";
  }
}

GLuint LoadShaders() {
  std::cout << "[LOG] LoadShaders called.\n";
  std::string vertCode = ReadFile("bin/world.vert");
  if (vertCode.empty())
    vertCode = ReadFile("world.vert");

  std::string fragCode = ReadFile("bin/world.frag");
  if (fragCode.empty())
    fragCode = ReadFile("world.frag");

  if (vertCode.empty() || fragCode.empty()) {
    printf("[ERROR] Shaders not found!\n");
    return 0;
  }

  const char *vShaderCode = vertCode.c_str();
  const char *fShaderCode = fragCode.c_str();

  GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex, 1, &vShaderCode, NULL);
  glCompileShader(vertex);
  checkShader(vertex, "VERTEX");

  GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment, 1, &fShaderCode, NULL);
  glCompileShader(fragment);
  checkShader(fragment, "FRAGMENT");

  GLuint program = glCreateProgram();
  glAttachShader(program, vertex);
  glAttachShader(program, fragment);
  glLinkProgram(program);
  checkProgram(program);

  glDeleteShader(vertex);
  glDeleteShader(fragment);
  return program;
}

// Hydrology Warm-Up (100 ticks for prominent rivers)
void MasterRegenerate(WorldBuffers &buffers, WorldSettings &settings,
                      TerrainController &terrain, NeighborFinder &finder,
                      NeighborGraph &graph) {

  if (buffers.flux)
    std::fill_n(buffers.flux, buffers.count, 0.0f);
  if (buffers.nextFlux)
    std::fill_n(buffers.nextFlux, buffers.count, 0.0f);
  if (buffers.population)
    std::fill_n(buffers.population, buffers.count, 0u);
  if (buffers.factionID)
    std::fill_n(buffers.factionID, buffers.count, 0);

  terrain.GenerateProceduralTerrain(buffers, settings);
  finder.BuildGraph(buffers, buffers.count, graph);

  if (settings.erosionIterations > 0) {
    terrain.ApplyThermalErosion(buffers, settings.erosionIterations);
  }

  if (graph.neighborData) {
    for (int i = 0; i < 200; ++i) {
      HydrologySim::Update(buffers, graph);
    }
  }
}

// --- GLOBALS ---
static int g_viewMode = 0;
static int g_hoveredIndex = -1;
static bool g_showDBEditor = false;
static int g_activeTab = 0;
static bool g_isPaused = true;

// Paint Tool Settings
static int g_paintMode = 0; // 0=Raise, 1=Lower, 2=Smooth
static float g_brushSize = 25.0f;
static float g_brushSpeed = 0.01f;

// Forward declaration for EditorUI
void DrawDatabaseEditor(bool *p_open);

// --- The God Mode Dashboard (Tabbed Layout) ---
void DrawGodModeUI(WorldSettings &settings, WorldBuffers &buffers,
                   TerrainController &terrain, NeighborFinder &finder,
                   NeighborGraph &graph, int screenW, int screenH) {

  ImGui::SetNextWindowPos(ImVec2(0, 0));
  ImGui::SetNextWindowSize(ImVec2((float)screenW / 3.0f, (float)screenH));

  ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                           ImGuiWindowFlags_NoTitleBar;
  ImGui::Begin("Omnis Control", nullptr, flags);

  ImGui::Text("OMNIS ENGINE | %d Cells", buffers.count);
  ImGui::Separator();

  // --- TABBED WORKFLOW ---
  int prevTab = g_activeTab;
  if (ImGui::BeginTabBar("ControlTabs")) {

    // TAB 1: ARCHITECT (Painting & Terrain)
    if (ImGui::BeginTabItem("Architect")) {
      g_activeTab = 0;

      ImGui::TextColored(ImVec4(0.5f, 1.0f, 1.0f, 1.0f), "Global Parameters");
      ImGui::InputInt("Seed", &settings.seed);
      if (ImGui::Button("Randomize Seed"))
        settings.seed = rand();

      ImGui::Separator();
      ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.5f, 1.0f), "Shape & Form");
      ImGui::SliderFloat("Land Mass", &settings.continentFreq, 0.001f, 0.02f,
                         "Size: %.4f");
      ImGui::SliderFloat("Sea Level", &settings.seaLevel, 0.0f, 1.0f);
      ImGui::Checkbox("Island Mode", &settings.islandMode);
      if (settings.islandMode) {
        ImGui::SliderFloat("Edge Falloff", &settings.edgeFalloff, 0.05f, 0.5f);
        ImGui::SliderFloat("Edge Margin", &settings.islandMargin, 0.0f, 200.0f);
      }

      ImGui::Separator();
      ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Sculpting Tools");
      ImGui::RadioButton("Raise", &g_paintMode, 0);
      ImGui::SameLine();
      ImGui::RadioButton("Lower", &g_paintMode, 1);
      ImGui::SameLine();
      ImGui::RadioButton("Smooth", &g_paintMode, 2);
      ImGui::SliderFloat("Brush Size", &g_brushSize, 1.0f, 100.0f);
      ImGui::SliderFloat("Brush Speed", &g_brushSpeed, 0.001f, 0.1f);

      if (ImGui::Button("REGENERATE WORLD", ImVec2(-1, 40))) {
        MasterRegenerate(buffers, settings, terrain, finder, graph);
      }

      ImGui::Separator();
      ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "Persistence");
      if (ImGui::Button("QUICK SAVE", ImVec2(120, 30)))
        AssetManager::SaveSimulationState("saves/quicksave.bin", buffers,
                                          settings);
      ImGui::SameLine();
      if (ImGui::Button("QUICK LOAD", ImVec2(120, 30)))
        AssetManager::LoadSimulationState("saves/quicksave.bin", buffers,
                                          settings);

      ImGui::EndTabItem();
    }

    // TAB 2: SIM & SETUP (Climate & Time)
    if (ImGui::BeginTabItem("Sim & Setup")) {
      g_activeTab = 1;

      ImGui::TextColored(ImVec4(0.5f, 1.0f, 1.0f, 1.0f),
                         "3-Zone Climate Config");

      if (ImGui::CollapsingHeader("North Pole Zone")) {
        ImGui::SliderFloat("North Wind Dir", &settings.windDirNorth, 0.0f,
                           6.28f);
        ImGui::SliderFloat("North Wind Force", &settings.windStrengthNorth,
                           0.0f, 1.0f);
        ImGui::SliderFloat("North Temp Offset", &settings.tempOffsetNorth,
                           -0.5f, 0.5f);
      }
      if (ImGui::CollapsingHeader("Equatorial Zone")) {
        ImGui::SliderFloat("Equator Wind Dir", &settings.windDirEquator, 0.0f,
                           6.28f);
        ImGui::SliderFloat("Equator Wind Force", &settings.windStrengthEquator,
                           0.0f, 1.0f);
      }
      if (ImGui::CollapsingHeader("South Pole Zone")) {
        ImGui::SliderFloat("South Wind Dir", &settings.windDirSouth, 0.0f,
                           6.28f);
        ImGui::SliderFloat("South Wind Force", &settings.windStrengthSouth,
                           0.0f, 1.0f);
        ImGui::SliderFloat("South Temp Offset", &settings.tempOffsetSouth,
                           -0.5f, 0.5f);
      }

      ImGui::Separator();
      ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Simulation Control");
      if (ImGui::Button(g_isPaused ? "RESUME TIME" : "PAUSE TIME",
                        ImVec2(-1, 40)))
        g_isPaused = !g_isPaused;

      ImGui::SliderInt("Time Scale", &settings.timeScale, 1, 10, "%dx Speed");

      ImGui::Separator();
      ImGui::Checkbox("Simulate Vegetation", &settings.enableBiology);
      ImGui::Checkbox("Enable Factions", &settings.enableFactions);
      ImGui::Checkbox("Enable Conflict", &settings.enableConflict);

      if (ImGui::Button("Spawn Civilizations", ImVec2(-1, 30))) {
        for (int k = 0; k < 5; ++k)
          AgentSystem::SpawnCivilization(buffers, k + 1);
      }

      ImGui::EndTabItem();
    }

    // TAB 3: VISUALS
    if (ImGui::BeginTabItem("Visuals")) {
      g_activeTab = 2;
      const char *zoomLevels[] = {"Global", "Regional", "Local", "Cell"};
      ImGui::Combo("Zoom Level", &settings.zoomLevel, zoomLevels, 4);
      ImGui::SliderFloat2("View Offset", settings.viewOffset, 0.0f, 1.0f);

      const char *modes[] = {"Terrain (Biomes)", "Chaos (Mana)",
                             "Economy (Wealth)"};
      ImGui::Combo("View Mode", &g_viewMode, modes, 3);
      ImGui::EndTabItem();
    }

    ImGui::EndTabBar();
  }

  // --- AUTOMATION: TAB TRANSITIONS ---
  if (prevTab == 0 && g_activeTab == 1) { // Left Architect -> Entering Sim
    AssetManager::SaveSimulationState("saves/sim_start.bin", buffers, settings);
    std::cout << "[WORKFLOW] Autosaved sim_start.bin. Simulation Ready.\n";
  }
  if (prevTab == 1 && g_activeTab != 1) { // Left Sim -> Pausing
    g_isPaused = true;
    AssetManager::SaveSimulationState("saves/sim_end.bin", buffers, settings);
    std::cout << "[WORKFLOW] Autosaved sim_end.bin. Simulation Paused.\n";
  }

  // --- CELL INSPECTOR ---
  ImGui::Separator();
  ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "CELL INSPECTOR");
  if (g_hoveredIndex != -1 && g_hoveredIndex < (int)buffers.count) {
    ImGui::Text("ID: %d | H: %.3f | T: %.3f | R: %.3f", g_hoveredIndex,
                buffers.height[g_hoveredIndex],
                buffers.temperature[g_hoveredIndex],
                buffers.moisture[g_hoveredIndex]);
    if (buffers.population)
      ImGui::Text("Pop: %u | Faction: %d", buffers.population[g_hoveredIndex],
                  buffers.factionID[g_hoveredIndex]);
  } else {
    ImGui::TextDisabled("(Hover over map to inspect)");
  }

  ImGui::End();
}

int main() {
  std::cout << "[LOG] Starting Main...\n";
  if (!glfwInit()) {
    std::cout << "[ERROR] GLFW Init Failed!\n";
    return -1;
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  // Center window logic (approximate)
  const GLFWvidmode *mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
  int windowW = 1600;
  int windowH = 900;

  GLFWwindow *window =
      glfwCreateWindow(windowW, windowH, "Omnis World Engine", NULL, NULL);
  if (!window) {
    std::cout << "[ERROR] Window Creation Failed!\n";
    return -1;
  }

  if (mode) {
    glfwSetWindowPos(window, (mode->width - windowW) / 2,
                     (mode->height - windowH) / 2);
  }

  glfwMakeContextCurrent(window);
  glewInit();
  glEnable(GL_PROGRAM_POINT_SIZE);

  // 2. Initialize Core Systems
  WorldBuffers buffers;
  buffers.Initialize(1000000);
  WorldSettings settings;

  // Random seed on startup
  srand((unsigned int)time(NULL));
  settings.seed = rand();

  // Hydrology Systems
  NeighborGraph graph;
  NeighborFinder finder;

  TerrainController terrain;
  MapRenderer renderer;
  renderer.Setup(buffers);

  GLuint shaderProgram = LoadShaders();

  MasterRegenerate(buffers, settings, terrain, finder, graph);

  // Initialize Subsystems
  AgentSystem::Initialize();
  LoreScribeNS::Initialize();
  AssetManager::Initialize();

  // Setup ImGui
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 330");

  // Style
  ImGui::StyleColorsDark();

  // 5. Main Loop
  std::cout << "[LOG] Entering Main Loop...\n";
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    int winW, winH;
    glfwGetWindowSize(window, &winW, &winH);

    int fbW, fbH;
    glfwGetFramebufferSize(window, &fbW, &fbH);

    // Mouse to Inspector & Painting
    double mx, my;
    glfwGetCursorPos(window, &mx, &my);
    float uiWidth = (float)winW / 3.0f;
    float mapStartX = uiWidth;
    float mapWidth = (float)winW - uiWidth;

    if (mx > mapStartX && mx < winW && my > 0 && my < winH) {
      float normX = (float)(mx - mapStartX) / mapWidth;
      float normY = 1.0f - (float)(my / (float)winH);
      int mapX = (int)(normX * 1000);
      int mapY = (int)(normY * 1000);
      if (mapX >= 0 && mapX < 1000 && mapY >= 0 && mapY < 1000)
        g_hoveredIndex = mapY * 1000 + mapX;
      else
        g_hoveredIndex = -1;

      // PAINTING LOGIC
      if (g_activeTab == 0 &&
          glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS &&
          !ImGui::GetIO().WantCaptureMouse) {
        if (g_hoveredIndex != -1) {
          if (g_paintMode == 0)
            terrain.RaiseTerrain(buffers, g_hoveredIndex, g_brushSize,
                                 g_brushSpeed);
          else if (g_paintMode == 1)
            terrain.LowerTerrain(buffers, g_hoveredIndex, g_brushSize,
                                 g_brushSpeed);
          else if (g_paintMode == 2)
            terrain.SmoothTerrain(buffers, g_hoveredIndex, g_brushSize,
                                  g_brushSpeed);
        }
      }
    } else
      g_hoveredIndex = -1;

    // SIMULATION TICK
    if (g_activeTab == 1 && !g_isPaused && graph.neighborData) {
      for (int tick = 0; tick < settings.timeScale; ++tick) {
        ClimateSim::Update(buffers, settings);
        HydrologySim::Update(buffers, graph);
        AgentSystem::UpdateBiology(buffers, settings);
        if (settings.enableFactions)
          AgentSystem::UpdateCivilization(buffers, graph);
        if (settings.enableConflict)
          ConflictSystem::Update(buffers, graph, settings);
        UnitSystem::Update(buffers, 100);
        CivilizationSim::Update(buffers, graph, settings);
      }
    }

    // RENDER
    float dpiX = (float)fbW / (float)winW;
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    DrawGodModeUI(settings, buffers, terrain, finder, graph, winW, winH);
    if (g_showDBEditor)
      DrawDatabaseEditor(&g_showDBEditor);

    glViewport((int)(uiWidth * dpiX), 0, (int)((winW - uiWidth) * dpiX), fbH);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(shaderProgram);
    glUniform1f(glGetUniformLocation(shaderProgram, "u_zoom"),
                (float)pow(4.0, settings.zoomLevel));
    glUniform2f(glGetUniformLocation(shaderProgram, "u_offset"),
                settings.viewOffset[0], settings.viewOffset[1]);
    glUniform1f(glGetUniformLocation(shaderProgram, "u_pointSize"),
                settings.pointSize);
    renderer.UpdateVisuals(buffers, settings, g_viewMode);
    renderer.Render(settings);

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window);
  }

  finder.Cleanup(graph);
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}
