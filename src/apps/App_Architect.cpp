#define GLEW_STATIC
#define GLFW_INCLUDE_NONE
#include "../../deps/imgui/backends/imgui_impl_glfw.h"
#include "../../deps/imgui/backends/imgui_impl_opengl3.h"
#include "../../deps/imgui/imgui.h"
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
#include "../../include/AssetManager.hpp"
#include "../../include/BinaryExporter.hpp"
#include "../../include/Biology.hpp"
#include "../../include/Environment.hpp"
#include "../../include/Lore.hpp"
#include "../../include/PlatformUtils.hpp"
#include "../../include/Simulation.hpp"
#include "../../include/Terrain.hpp"
#include "../../include/WorldEngine.hpp"


// Visuals
#include "../frontend/GuiController.hpp"
#include "../frontend/MapRenderer.hpp"

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
  // The Architect EXE is in bin/ , so we look for shaders in bin/shaders/ or
  // shaders/
  std::string vertCode = ReadFile("bin/shaders/world.vert");
  if (vertCode.empty())
    vertCode = ReadFile("shaders/world.vert");

  std::string fragCode = ReadFile("bin/shaders/world.frag");
  if (fragCode.empty())
    fragCode = ReadFile("shaders/world.frag");

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

void MasterRegenerate(WorldBuffers &buffers, WorldSettings &settings,
                      TerrainController &terrain, NeighborFinder &finder,
                      NeighborGraph &graph) {
  if (buffers.flux)
    std::fill_n(buffers.flux, buffers.count, 0.0f);
  if (buffers.nextFlux)
    std::fill_n(buffers.nextFlux, buffers.count, 0.0f);

  terrain.GenerateProceduralTerrain(buffers, settings);
  finder.BuildGraph(buffers, buffers.count, graph);

  if (settings.erosionIterations > 0) {
    terrain.ApplyThermalErosion(buffers, settings.erosionIterations);
  }

  if (graph.neighborData) {
    for (int i = 0; i < 50; ++i) {
      HydrologySim::Update(buffers, graph, settings);
    }
  }
}

int main() {
  std::cout << "[LOG] Starting Omnis Architect...\n";
  if (!glfwInit())
    return -1;

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  int windowW = 1600;
  int windowH = 900;
  GLFWwindow *window =
      glfwCreateWindow(windowW, windowH, "Omnis Architect v2", NULL, NULL);
  if (!window)
    return -1;

  glfwMakeContextCurrent(window);
  glewInit();
  glEnable(GL_PROGRAM_POINT_SIZE);

  WorldBuffers buffers;
  buffers.Initialize(1000000);
  WorldSettings settings;
  srand((unsigned int)time(NULL));
  settings.seed = rand();

  NeighborGraph graph;
  NeighborFinder finder;
  TerrainController terrain;
  MapRenderer renderer;
  renderer.Setup(buffers);

  GLuint shaderProgram = LoadShaders();
  MasterRegenerate(buffers, settings, terrain, finder, graph);

  AssetManager::Initialize();
  GuiController::Initialize();

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 330");
  ImGui::StyleColorsDark();

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    int winW, winH, fbW, fbH;
    glfwGetWindowSize(window, &winW, &winH);
    glfwGetFramebufferSize(window, &fbW, &fbH);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGuiIO &io = ImGui::GetIO();

    // Zoom/Pan logic
    if (io.MouseWheel != 0.0f && !io.WantCaptureMouse) {
      settings.zoomLevel += io.MouseWheel * 0.1f;
      if (settings.zoomLevel < 0.0f)
        settings.zoomLevel = 0.0f;
    }

    if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
      ImVec2 delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Middle);
      ImGui::ResetMouseDragDelta(ImGuiMouseButton_Middle);
      float zoom = pow(4.0f, settings.zoomLevel);
      float panSpeed = 1.0f / (1000.0f * zoom);
      settings.viewOffset[0] -= delta.x * panSpeed * 2.0f;
      settings.viewOffset[1] += delta.y * panSpeed * 2.0f;
    }

    if (io.MouseDown[0] && !io.WantCaptureMouse) {
      renderer.isDirty = true;
    }

    GuiState guiState = GuiController::DrawMainLayout(
        buffers, settings, terrain, graph, winW, winH);
    if (guiState.requiresRedraw)
      renderer.isDirty = true;

    // RENDER
    float dpiX = (float)fbW / (float)winW;
    float dpiY = (float)fbH / (float)winH;
    int glViewY = winH - (guiState.viewY + guiState.viewH);
    glViewport((int)(guiState.viewX * dpiX), (int)(glViewY * dpiY),
               (int)(guiState.viewW * dpiX), (int)(guiState.viewH * dpiY));

    glClearColor(0.1f, 0.1f, 0.15f, 1);
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
