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

// --- The God Mode Dashboard ---
void DrawGodModeUI(WorldSettings &settings, WorldBuffers &buffers,
                   TerrainController &terrain, int screenW, int screenH) {

  // Docking Layout logic
  float uiWidth = (float)screenW / 3.0f;
  ImGui::SetNextWindowPos(ImVec2(0, 0));
  ImGui::SetNextWindowSize(ImVec2(uiWidth, (float)screenH));

  ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                           ImGuiWindowFlags_NoCollapse |
                           ImGuiWindowFlags_NoTitleBar;

  ImGui::Begin("God Mode Dashboard", nullptr, flags);

  ImGui::TextDisabled("OMNIS WORLD ENGINE v0.1");
  ImGui::Separator();

  // 1. Perspective Control
  if (ImGui::CollapsingHeader("Perspective & Scale",
                              ImGuiTreeNodeFlags_DefaultOpen)) {
    const char *zoomLevels[] = {"Global", "Quadrant", "Regional", "Local"};
    if (ImGui::Combo("View Level", &settings.zoomLevel, zoomLevels, 4)) {
      settings.pointSize = (settings.zoomLevel == 0)   ? 1.0f
                           : (settings.zoomLevel == 1) ? 4.0f
                           : (settings.zoomLevel == 2) ? 16.0f
                                                       : 64.0f;
    }
    ImGui::SliderFloat2("View Offset", settings.viewOffset, 0.0f, 1.0f);
    ImGui::SliderFloat("Point Size", &settings.pointSize, 1.0f, 128.0f);
    ImGui::Text("Rendered Points: %d", (int)(buffers.count));
  }

  // 2. Core Generation Settings
  if (ImGui::CollapsingHeader("Generation Settings",
                              ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::InputInt("Seed", &settings.seed);

    ImGui::Separator();
    ImGui::Text("Terrain Shape");
    ImGui::SliderFloat("Height Multiplier", &settings.heightMultiplier, 0.1f,
                       10.0f);
    ImGui::SliderFloat("Height Severity (Roughness)", &settings.heightSeverity,
                       0.1f, 5.0f);
    ImGui::SliderFloat("Feature Frequency", &settings.featureFrequency, 0.001f,
                       0.1f);
    ImGui::SliderFloat("Clustering", &settings.featureClustering, 1.0f, 5.0f);

    ImGui::Separator();
    ImGui::Text("Water & Levels");
    ImGui::SliderFloat("Sea Level", &settings.seaLevel, 0.0f, 1.0f);
    ImGui::SliderFloat("Height Min", &settings.heightMin, 0.0f, 1.0f);
    ImGui::SliderFloat("Height Max", &settings.heightMax, 0.0f, 1.0f);

    if (ImGui::Button("Regenerate Terrain", ImVec2(-1, 0))) {
      terrain.GenerateProceduralTerrain(buffers, settings);
    }
  }

  // 3. Climate & Atmosphere
  if (ImGui::CollapsingHeader("Atmosphere & Climate")) {
    ImGui::SliderFloat("Global Temp", &settings.globalTempModifier, 0.5f, 2.0f);
    ImGui::SliderFloat("Rainfall Mod", &settings.rainfallModifier, 0.0f, 5.0f);

    ImGui::Separator();
    ImGui::Checkbox("Manual Wind Override", &settings.manualWindOverride);
    if (settings.manualWindOverride) {
      ImGui::SliderFloat("Wind Strength", &settings.globalWindStrength, 0.0f,
                         5.0f);
      ImGui::SliderFloat("Zone 1 (N.Pole)", &settings.windZones[0], -1.0f,
                         1.0f);
      ImGui::SliderFloat("Zone 2", &settings.windZones[1], -1.0f, 1.0f);
      ImGui::SliderFloat("Zone 3 (Equator)", &settings.windZones[2], -1.0f,
                         1.0f);
      ImGui::SliderFloat("Zone 4", &settings.windZones[3], -1.0f, 1.0f);
      ImGui::SliderFloat("Zone 5 (S.Pole)", &settings.windZones[4], -1.0f,
                         1.0f);
    }
  }

  // 4. Geology & Erosion
  if (ImGui::CollapsingHeader("Geology & Erosion")) {
    ImGui::SliderInt("Erosion Iterations", &settings.erosionIterations, 1, 100);
    if (ImGui::Button("Run Thermal Erosion", ImVec2(-1, 0))) {
      terrain.ApplyThermalErosion(buffers, settings.erosionIterations);
    }
    if (ImGui::Button("Import Heightmap", ImVec2(-1, 0))) {
      std::string path = PlatformUtils::OpenFileDialog();
      if (!path.empty()) {
        terrain.LoadFromImage(path.c_str(), buffers);
      }
    }
  }

  // 5. Civilization / Factions
  if (ImGui::CollapsingHeader("Civilization")) {
    ImGui::Checkbox("Enable Factions", &settings.enableFactions);
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

  TerrainController terrain;
  MapRenderer renderer;
  renderer.Setup(buffers);

  GLuint shaderProgram = LoadShaders();

  // Generate Initial World
  terrain.GenerateProceduralTerrain(buffers, settings);

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

    // Calculate DPI Scale
    float dpiScaleX = (winW > 0) ? ((float)fbW / (float)winW) : 1.0f;
    float dpiScaleY = (winH > 0) ? ((float)fbH / (float)winH) : 1.0f;

    // UI (Uses Logical Window Coordinates)
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Pass Logical Size (winW, winH) to ImGui
    DrawGodModeUI(settings, buffers, terrain, winW, winH);

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

    renderer.UpdateVisuals(buffers, settings);
    renderer.Render(settings);

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window);
  }

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}
