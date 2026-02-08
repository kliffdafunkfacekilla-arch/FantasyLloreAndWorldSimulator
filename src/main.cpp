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

// Modular Headers
#include "../include/AssetManager.hpp"
#include "../include/Biology.hpp"
#include "../include/Environment.hpp"
#include "../include/Lore.hpp"
#include "../include/PlatformUtils.hpp"
#include "../include/Simulation.hpp"
#include "../include/Terrain.hpp"
#include "../include/WorldEngine.hpp"

// Visuals
#include "frontend/GuiController.hpp"
#include "frontend/MapRenderer.hpp"

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
      HydrologySim::Update(buffers, graph, settings);
    }
  }
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
  GuiController::Initialize();

  // Setup ImGui
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 330");

  ImGui::StyleColorsDark();

  // 5. Main Loop
  std::cout << "[LOG] Entering Main Loop...\n";
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    int winW, winH;
    glfwGetWindowSize(window, &winW, &winH);
    int fbW, fbH;
    glfwGetFramebufferSize(window, &fbW, &fbH);

    // --- GUI AND INPUT ---
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGuiIO &io = ImGui::GetIO();

    // 1. ZOOM HANDLING (Scroll Wheel)
    float wheel = io.MouseWheel;
    static int currentZoomIdx = 1;
    float zoomLevels[] = {0.2f, 1.0f, 2.5f,
                          5.0f}; // Planet, Region, Local, Detail

    if (wheel != 0.0f && !io.WantCaptureMouse) {
      if (wheel > 0 && currentZoomIdx < 3)
        currentZoomIdx++;
      else if (wheel < 0 && currentZoomIdx > 0)
        currentZoomIdx--;

      settings.zoomLevel = zoomLevels[currentZoomIdx];
    }

    // 1.5 PAN HANDLING (Middle Mouse Drag)
    if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
      ImVec2 delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Middle);
      // Reset delta so we get incremental updates
      ImGui::ResetMouseDragDelta(ImGuiMouseButton_Middle);

      // Convert screen delta to world delta
      // WorldDelta = -ScreenDelta / (ViewportSize * Zoom)
      // We need 'guiState' for ViewportSize, but we haven't calculated it yet
      // this frame. We can approximate or perform this Logic AFTER
      // DrawMainLayout? Or just use WinW * 0.7f (approx viewport width) Let's
      // defer Panning update to AFTER DrawMainLayout or use current 'fbW'
      // assuming full width for now. Better: Use calculated guiState from
      // previous frame? Actually, just using a sensitive constant is often
      // enough for "feeling".

      float zoom = pow(4.0f, settings.zoomLevel);
      float panSpeed = 1.0f / (1000.0f * zoom); // 1000 approx pixels

      // Invert X/Y because dragging mouse Right should move View Left (Offset
      // decreases) Wait, Offset is "Camera Position" essentially. If I move
      // Camera Right (Increase Offset), the World moves Left. If I drag Mouse
      // Right, I want World to move Right. So I need to DECREASE Offset.

      settings.viewOffset[0] -= delta.x * panSpeed * 2.0f; // *2.0 for speed
      settings.viewOffset[1] +=
          delta.y * panSpeed * 2.0f; // Dragging Up moves map Up (Decrease Y
                                     // offset? No, Render Y is flipped)

      // Clamp
      // settings.viewOffset[0] = std::max(0.0f, std::min(1.0f,
      // settings.viewOffset[0])); settings.viewOffset[1] = std::max(0.0f,
      // std::min(1.0f, settings.viewOffset[1])); Actually, let's not clamp
      // tightly yet, to allow overscroll.
    }

    // 2. MOUSE BRUSH LOGIC
    // If we are clicking on the map (not UI), set dirty flag because
    // GuiController might have changed terrain
    if (io.MouseDown[0] && !io.WantCaptureMouse) {
      renderer.isDirty = true;
    }

    // Draw the new Command Center Layout
    GuiState guiState = GuiController::DrawMainLayout(
        buffers, settings, terrain, graph, winW, winH);

    // 2.5 HANDLE REDRAW REQUESTS
    if (guiState.requiresRedraw) {
      renderer.isDirty = true;
    }

    // --- SIMULATION TICK ---
    // Runs only if Tab "Life & Civ" is active (id=1) and not paused
    if (guiState.activeTab == 1 && !guiState.isPaused && graph.neighborData) {
      for (int tick = 0; tick < settings.timeScale; ++tick) {
        ClimateSim::Update(buffers, settings);
        HydrologySim::Update(buffers, graph, settings);
        AgentSystem::UpdateBiology(buffers, settings);
        if (settings.enableFactions)
          AgentSystem::UpdateCivilization(buffers, graph);
        if (settings.enableConflict)
          ConflictSystem::Update(buffers, graph, settings);
        UnitSystem::Update(buffers, 100);
        CivilizationSim::Update(buffers, graph, settings);
      }
    }

    // --- RENDER ---
    // Calculate Viewport based on GUI layout
    // Convert GUI coords (Window Space) to Framebuffer Space (DPI aware)
    float dpiX = (float)fbW / (float)winW;
    float dpiY = (float)fbH / (float)winH;

    // Viewport Y in OpenGL is from Bottom-Left.
    // GUI Y is from Top-Left.
    // guiState.viewY is distance from top.
    // OpenGL Y = winH - (guiState.viewY + guiState.viewH)

    int glViewY = winH - (guiState.viewY + guiState.viewH);

    glViewport((int)(guiState.viewX * dpiX), (int)(glViewY * dpiY),
               (int)(guiState.viewW * dpiX), (int)(guiState.viewH * dpiY));

    glClearColor(0.1f, 0.1f, 0.1f, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shaderProgram);
    glUniform1f(glGetUniformLocation(shaderProgram, "u_zoom"),
                (float)pow(4.0, settings.zoomLevel));
    glUniform2f(glGetUniformLocation(shaderProgram, "u_offset"),
                settings.viewOffset[0], settings.viewOffset[1]);
    glUniform1f(glGetUniformLocation(shaderProgram, "u_pointSize"),
                settings.pointSize);

    renderer.UpdateVisuals(buffers, settings, guiState.viewMode);
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
