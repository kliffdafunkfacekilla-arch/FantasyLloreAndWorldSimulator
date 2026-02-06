#define GLEW_STATIC
#define GLFW_INCLUDE_NONE
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cmath>
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

// --- Master Regenerate Pipeline ---
void MasterRegenerate(WorldBuffers &buffers, WorldSettings &settings,
                      TerrainController &terrain, NeighborFinder &finder,
                      NeighborGraph &graph) {

  // 1. Reset Sim State
  if (buffers.flux)
    std::fill_n(buffers.flux, buffers.count, 0.0f);
  if (buffers.nextFlux)
    std::fill_n(buffers.nextFlux, buffers.count, 0.0f);
  if (buffers.population)
    std::fill_n(buffers.population, buffers.count, 0u);
  if (buffers.factionID)
    std::fill_n(buffers.factionID, buffers.count, 0);

  // 2. Shape the Land
  terrain.GenerateProceduralTerrain(buffers, settings);

  // 3. Connect the Graph
  finder.BuildGraph(buffers, buffers.count, graph);

  // 4. Thermal Erosion (optional)
  if (settings.erosionIterations > 0) {
    terrain.ApplyThermalErosion(buffers, settings.erosionIterations);
  }

  // 5. Hydrology Warm-Up (30 ticks so rivers appear immediately)
  if (graph.neighborData) {
    for (int i = 0; i < 30; ++i) {
      HydrologySim::Update(buffers, graph);
    }
  }
}

// Global view mode for renderer (0=Terrain, 1=Chaos, 2=Economy)
static int g_viewMode = 0;

// Global inspector state (hover cell ID)
static int g_hoveredIndex = -1;

// Database Editor state
static bool g_showDBEditor = false;

// Forward declaration for EditorUI
void DrawDatabaseEditor(bool *p_open);

// --- The God Mode Dashboard (Tabbed Layout) ---
void DrawGodModeUI(WorldSettings &settings, WorldBuffers &buffers,
                   TerrainController &terrain, NeighborGraph &graph,
                   NeighborFinder &finder, int screenW, int screenH) {

  ImGui::SetNextWindowPos(ImVec2(0, 0));
  ImGui::SetNextWindowSize(ImVec2((float)screenW / 3.0f, (float)screenH));

  ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                           ImGuiWindowFlags_NoTitleBar;
  ImGui::Begin("Omnis Control", nullptr, flags);

  ImGui::Text("OMNIS ENGINE | %d Cells", buffers.count);

  // --- THE MASTER BUTTON ---
  ImGui::Spacing();
  if (ImGui::Button("GENERATE NEW WORLD", ImVec2(-1, 40))) {
    MasterRegenerate(buffers, settings, terrain, finder, graph);
  }

  // --- DATABASE EDITOR BUTTON ---
  if (ImGui::Button("EDIT RULES / JSON", ImVec2(-1, 30))) {
    g_showDBEditor = !g_showDBEditor;
  }
  ImGui::Spacing();
  ImGui::Separator();

  if (ImGui::BeginTabBar("ControlTabs")) {

    // TAB 1: THE ARCHITECT (Generation)
    if (ImGui::BeginTabItem("Architect")) {

      ImGui::TextColored(ImVec4(0.5f, 1.0f, 1.0f, 1.0f), "Global Parameters");
      ImGui::InputInt("Seed", &settings.seed);
      if (ImGui::Button("Randomize Seed"))
        settings.seed = rand();

      ImGui::Separator();
      ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.5f, 1.0f), "Shape & Form");

      ImGui::SliderFloat("Land Mass", &settings.continentFreq, 0.001f, 0.02f,
                         "Size: %.4f");
      ImGui::SliderFloat("Sea Level", &settings.seaLevel, 0.0f, 1.0f);
      ImGui::SliderInt("Erosion Passes", &settings.erosionIterations, 0, 50);

      ImGui::Separator();
      ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "Features");
      ImGui::SliderFloat("Mtn Coverage", &settings.featureFrequency, 0.01f,
                         0.1f);
      ImGui::SliderFloat("Mtn Height", &settings.mountainInfluence, 0.0f, 3.0f);
      ImGui::SliderFloat("Warping", &settings.warpStrength, 0.0f, 2.0f);
      ImGui::SliderFloat("Terracing", &settings.terraceSteps, 0.0f, 2.0f);

      ImGui::Separator();
      ImGui::Checkbox("Island Mode", &settings.islandMode);
      if (settings.islandMode) {
        ImGui::SliderFloat("Edge Falloff", &settings.edgeFalloff, 0.05f, 0.5f);
      }

      ImGui::Separator();
      ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Manual Controls");

      // Manual triggers for granular control
      if (ImGui::Button("Re-Shape Only"))
        terrain.GenerateProceduralTerrain(buffers, settings);
      ImGui::SameLine();
      if (ImGui::Button("Erode Only"))
        terrain.ApplyThermalErosion(buffers, 1);

      if (ImGui::Button("Import Heightmap")) {
        std::string path = PlatformUtils::OpenFileDialog();
        if (!path.empty())
          terrain.LoadFromImage(path.c_str(), buffers);
      }
      ImGui::SameLine();
      if (ImGui::Button("Invert"))
        terrain.InvertHeights(buffers);

      ImGui::EndTabItem();
    }

    // TAB 2: THE GOD (Simulation)
    if (ImGui::BeginTabItem("Simulation")) {

      // --- MASTER TIME CONTROLS ---
      ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Chronos Controls");
      static bool isPaused = true;
      if (ImGui::Button(isPaused ? "RESUME TIME" : "PAUSE TIME",
                        ImVec2(-1, 40)))
        isPaused = !isPaused;

      ImGui::SliderInt("Time Scale", &settings.timeScale, 1, 10, "%dx Speed");

      // Run simulation ticks when not paused
      if (!isPaused && graph.neighborData) {
        for (int tick = 0; tick < settings.timeScale; ++tick) {
          ClimateSim::Update(buffers, settings);
          HydrologySim::Update(buffers, graph);

          // Biology (vegetation/wildlife growth)
          AgentSystem::UpdateBiology(buffers, settings);

          if (settings.enableRealtimeErosion) {
            terrain.ApplyThermalErosion(buffers, 1);
          }

          if (settings.enableFactions) {
            AgentSystem::UpdateCivilization(buffers, graph);
          }

          // Conflict (war/border pushing)
          if (settings.enableConflict) {
            ConflictSystem::Update(buffers, graph, settings);
          }
        }
      }

      ImGui::Separator();

      // --- WEATHER & EROSION ---
      ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "Dynamic Weather");

      ImGui::SliderFloat("Wind Angle", &settings.windAngle, 0.0f, 6.28f,
                         "%.2f rad");
      ImGui::SliderFloat("Wind Force", &settings.windStrengthSim, 0.0f, 0.5f);

      ImGui::SliderFloat("Global Temp", &settings.globalTemperature, -0.5f,
                         0.5f);
      ImGui::SliderFloat("Global Rain", &settings.globalMoisture, -0.5f, 0.5f);

      ImGui::Separator();

      // --- DESTRUCTIVE FORCES ---
      ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "Natural Forces");
      ImGui::Checkbox("Enable Realtime Erosion",
                      &settings.enableRealtimeErosion);
      if (settings.enableRealtimeErosion) {
        ImGui::TextDisabled("Warning: Mountains will melt over time!");
      }

      if (graph.neighborData == nullptr) {
        ImGui::TextColored(ImVec4(1, 0.5f, 0, 1),
                           "Build graph first for simulation!");
      }

      ImGui::Separator();

      // --- POLITICS & WAR ---
      ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Politics & War");
      ImGui::Checkbox("Enable Conflict", &settings.enableConflict);

      if (ImGui::Button("Spawn 5 Kingdoms")) {
        for (int k = 0; k < 5; ++k) {
          AgentSystem::SpawnCivilization(buffers, k + 1);
        }
        LoreScribeNS::LogEvent(0, "GENESIS", 0,
                               "5 Civilizations have appeared.");
      }

      ImGui::Separator();

      // --- BIOLOGY ---
      ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "Biology");
      ImGui::Checkbox("Simulate Vegetation", &settings.enableBiology);

      // Human aggression tweaker
      static float humanAggro = 0.5f;
      if (ImGui::SliderFloat("Human Aggression", &humanAggro, 0.0f, 1.0f)) {
        if (!AgentSystem::speciesRegistry.empty()) {
          AgentSystem::speciesRegistry[0].aggression = humanAggro;
        }
      }

      ImGui::Separator();
      ImGui::Text("Civilization");
      ImGui::Checkbox("Enable Factions", &settings.enableFactions);
      if (settings.enableFactions) {
        if (ImGui::Button("Spawn Faction"))
          AgentSystem::SpawnCivilization(buffers, 1);
        ImGui::SameLine();
        if (ImGui::Button("Grow (1 Yr)")) {
          for (int i = 0; i < 36; ++i)
            AgentSystem::UpdateCivilization(buffers, graph);
        }
      }

      ImGui::Separator();

      // --- THE CHAOS ENGINE ---
      ImGui::TextColored(ImVec4(0.8f, 0.4f, 1.0f, 1.0f), "The Chaos Engine");

      if (ImGui::Button("Open Rift (Center)")) {
        // Spawn a rift at map center (approx 500,500 for 1M cells)
        int centerIdx = (int)(buffers.count / 2);
        ChaosField::SpawnRift(buffers, centerIdx, 1.0f);
        LoreScribeNS::LogEvent(0, "MAGIC", centerIdx,
                               "A Chaos Rift has opened!");
      }
      ImGui::SameLine();
      if (ImGui::Button("Clear All Chaos")) {
        if (buffers.chaos)
          std::fill_n(buffers.chaos, buffers.count, 0.0f);
        ChaosField::ClearRifts();
      }

      ImGui::Separator();

      // --- ECONOMY & LOGISTICS ---
      ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Economy & Logistics");
      if (ImGui::Button("Simulate Economy (10 ticks)")) {
        for (int i = 0; i < 10; ++i)
          LogisticsSystem::Update(buffers, graph);
      }

      ImGui::EndTabItem();
    }

    // TAB 3: THE VIEWER (Visuals)
    if (ImGui::BeginTabItem("Visuals")) {
      ImGui::Text("Camera");
      const char *zoomLevels[] = {"Global", "Quadrant", "Regional", "Local"};
      if (ImGui::Combo("View Level", &settings.zoomLevel, zoomLevels, 4)) {
        settings.pointSize = (settings.zoomLevel == 0)   ? 1.0f
                             : (settings.zoomLevel == 1) ? 4.0f
                             : (settings.zoomLevel == 2) ? 16.0f
                                                         : 64.0f;
      }
      ImGui::SliderFloat2("View Offset", settings.viewOffset, 0.0f, 1.0f);
      ImGui::SliderFloat("Point Size", &settings.pointSize, 1.0f, 128.0f);

      ImGui::Separator();
      ImGui::TextColored(ImVec4(0.5f, 1.0f, 1.0f, 1.0f), "Visualization Mode");
      static int viewMode = 0;
      g_viewMode = viewMode; // Sync to global
      ImGui::RadioButton("Terrain", &viewMode, 0);
      ImGui::SameLine();
      ImGui::RadioButton("Chaos", &viewMode, 1);
      ImGui::SameLine();
      ImGui::RadioButton("Economy", &viewMode, 2);

      ImGui::EndTabItem();
    }

    // TAB 4: RULES & DESIGN
    if (ImGui::BeginTabItem("Rules")) {
      // --- RESOURCE EDITOR ---
      ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Resource Registry");

      static int selectedRes = 0;
      if (AssetManager::resourceRegistry.size() > 0) {
        // Dropdown to select resource
        if (ImGui::BeginCombo(
                "Select Resource",
                AssetManager::resourceRegistry[selectedRes].name.c_str())) {
          for (size_t n = 0; n < AssetManager::resourceRegistry.size(); n++) {
            bool is_selected = (selectedRes == (int)n);
            if (ImGui::Selectable(
                    AssetManager::resourceRegistry[n].name.c_str(),
                    is_selected))
              selectedRes = (int)n;
            if (is_selected)
              ImGui::SetItemDefaultFocus();
          }
          ImGui::EndCombo();
        }

        // Edit Properties
        ResourceDef &r = AssetManager::resourceRegistry[selectedRes];

        // Name (read-only display)
        ImGui::Text("Name: %s", r.name.c_str());
        ImGui::SliderFloat("Value", &r.value, 0.1f, 20.0f);
        ImGui::SliderFloat("Scarcity", &r.scarcity, 0.0f, 1.0f);
        ImGui::Checkbox("Renewable", &r.isRenewable);

        ImGui::Text("Spawn Biomes:");
        ImGui::Checkbox("Forest", &r.spawnsInForest);
        ImGui::SameLine();
        ImGui::Checkbox("Mountain", &r.spawnsInMountain);
        ImGui::SameLine();
        ImGui::Checkbox("Desert", &r.spawnsInDesert);
        ImGui::SameLine();
        ImGui::Checkbox("Ocean", &r.spawnsInOcean);
      }

      ImGui::Separator();

      // --- CHAOS RULE EDITOR ---
      ImGui::TextColored(ImVec4(1.0f, 0.0f, 1.0f, 1.0f), "Chaos Laws");
      for (auto &rule : AssetManager::chaosRules) {
        ImGui::PushID(rule.name.c_str());
        ImGui::Text("%s", rule.name.c_str());
        ImGui::SliderFloat("Chance", &rule.probability, 0.0f, 0.1f, "%.4f");
        ImGui::SliderFloat("Threshold", &rule.minChaosLevel, 0.0f, 1.0f);
        ImGui::SliderFloat("Severity", &rule.severity, 0.0f, 1.0f);
        ImGui::PopID();
        ImGui::Separator();
      }

      // SAVE BUTTON
      if (ImGui::Button("SAVE ALL RULES TO JSON", ImVec2(-1, 40))) {
        AssetManager::SaveAll();
      }

      ImGui::EndTabItem();
    }

    ImGui::EndTabBar();
  }

  // --- CELL INSPECTOR ---
  ImGui::Separator();
  ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "CELL INSPECTOR");

  if (g_hoveredIndex != -1 && g_hoveredIndex < (int)buffers.count) {
    ImGui::Text("ID: %d", g_hoveredIndex);
    ImGui::Text("Height: %.3f", buffers.height[g_hoveredIndex]);
    if (buffers.temperature)
      ImGui::Text("Temp:   %.3f", buffers.temperature[g_hoveredIndex]);
    if (buffers.moisture)
      ImGui::Text("Rain:   %.3f", buffers.moisture[g_hoveredIndex]);
    if (buffers.population)
      ImGui::Text("Pop:    %u", buffers.population[g_hoveredIndex]);
    if (buffers.wealth)
      ImGui::Text("Gold:   %.2f", buffers.wealth[g_hoveredIndex]);
    if (buffers.chaos)
      ImGui::Text("Chaos:  %.2f", buffers.chaos[g_hoveredIndex]);
    if (buffers.infrastructure)
      ImGui::Text("Infra:  %.2f", buffers.infrastructure[g_hoveredIndex]);
    if (buffers.factionID)
      ImGui::Text("Faction:%d", buffers.factionID[g_hoveredIndex]);
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
  std::cout << "[LOG] GLEW Initialized.\n";
  glEnable(GL_PROGRAM_POINT_SIZE);

  // 2. Initialize Core Systems
  WorldBuffers buffers;
  buffers.Initialize(1000000);
  WorldSettings settings;

  // Hydrology Systems
  NeighborGraph graph;
  NeighborFinder finder;

  TerrainController terrain;
  MapRenderer renderer;
  renderer.Setup(buffers);

  GLuint shaderProgram = LoadShaders();

  // Generate Initial World
  terrain.GenerateProceduralTerrain(buffers, settings);

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

    // --- MOUSE TO CELL FOR INSPECTOR ---
    double mx, my;
    glfwGetCursorPos(window, &mx, &my);

    // Calculate map area (right 2/3 of window)
    float uiWidth = (float)winW / 3.0f;
    float mapStartX = uiWidth;
    float mapWidth = (float)winW - uiWidth;

    // Convert mouse to grid coords (1000x1000 grid for 1M cells)
    int gridW = 1000;
    if (mx > mapStartX && mx < winW && my > 0 && my < winH) {
      float normX = (float)(mx - mapStartX) / mapWidth;
      float normY = 1.0f - (float)(my / (float)winH); // Flip Y

      int mapX = (int)(normX * gridW);
      int mapY = (int)(normY * gridW);

      if (mapX >= 0 && mapX < gridW && mapY >= 0 && mapY < gridW) {
        g_hoveredIndex = mapY * gridW + mapX;
      } else {
        g_hoveredIndex = -1;
      }
    } else {
      g_hoveredIndex = -1;
    }

    // Calculate DPI Scale
    float dpiScaleX = (winW > 0) ? ((float)fbW / (float)winW) : 1.0f;
    float dpiScaleY = (winH > 0) ? ((float)fbH / (float)winH) : 1.0f;

    // UI (Uses Logical Window Coordinates)
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Pass Logical Size (winW, winH) to ImGui
    DrawGodModeUI(settings, buffers, terrain, graph, finder, winW, winH);

    // Draw Database Editor if open
    if (g_showDBEditor) {
      DrawDatabaseEditor(&g_showDBEditor);
    }

    // Render Logic (Uses Physical Framebuffer Coordinates)
    // We want the Map to start where the UI ends.
    // UI Width is winW / 3.0f (Logical)
    // So Map Offset is (winW / 3.0f) * dpiScaleX (Physical)

    int mapX = (int)((winW / 3.0f) * dpiScaleX);
    int mapW = fbW - mapX;

    glViewport(mapX, 0, mapW, fbH);

    // Clear Screen
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Activate Shaders and Render
    glUseProgram(shaderProgram);

    // Update Uniforms
    glUniform1f(glGetUniformLocation(shaderProgram, "u_zoom"),
                (float)pow(4.0, settings.zoomLevel));

    // We might need to adjust aspect ratio or offset logic since viewport
    // changed
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
