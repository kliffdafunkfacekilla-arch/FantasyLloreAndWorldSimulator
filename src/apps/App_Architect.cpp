#include "../../deps/imgui/backends/imgui_impl_glfw.h"
#include "../../deps/imgui/backends/imgui_impl_opengl3.h"
#include "../../deps/imgui/imgui.h"
#include "../../include/PlatformUtils.hpp"
#include "../../include/Terrain.hpp"
#include "../../include/WorldEngine.hpp"
#include "../../include/stb_image.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

// --- GLOBALS ---
WorldBuffers buffers;
WorldSettings settings;
float zoom = 1.0f;
float camX = 0.5f, camY = 0.5f;

// --- UI STATE ---
int brushMode = 0; // 0:Raise, 1:Lower, 2:Smooth
float brushSize = 5.0f;
float brushStrength = 0.05f;

void Setup() {
  buffers.Initialize(1000 * 1000); // 1 Million Cells
  TerrainController::GenerateHeightmap(buffers, settings);
  TerrainController::GenerateClimate(buffers, settings);
}

void DrawViewport() {
  ImGui::Begin("S.A.G.A. Viewport", nullptr,
               ImGuiWindowFlags_NoScrollbar |
                   ImGuiWindowFlags_NoScrollWithMouse);

  if (ImGui::IsWindowHovered()) {
    ImGuiIO &io = ImGui::GetIO();
    if (ImGui::IsMouseDown(0) && !io.WantCaptureMouse) {
      // Transform screen to world logic would go here
      // TerrainController::ApplyBrush(buffers, 1000, cx, cy, brushSize,
      // brushStrength, brushMode);
    }
    zoom += io.MouseWheel * 0.1f;
    if (zoom < 0.1f)
      zoom = 0.1f;
  }

  ImGui::Text("Map Scope: 1000x1000 | Render Scale: %.2f", zoom);
  ImGui::Separator();
  ImGui::Text("Middle Click to Pan | Scroll to Zoom | Left Click to Sculpt");

  ImGui::End();
}

void DrawTools() {
  ImGui::Begin("Architect Command Center");

  if (ImGui::BeginTabBar("ArchitectTabs")) {

    if (ImGui::BeginTabItem("Generation")) {
      ImGui::TextColored(ImVec4(0, 1, 1, 1), "Planetary Seeds");
      static int seed = 12345;
      ImGui::InputInt("Global Seed", &seed);
      if (ImGui::Button("Reset Landmass")) {
        settings.seed = seed;
        TerrainController::GenerateHeightmap(buffers, settings);
      }

      ImGui::Separator();
      ImGui::Text("Hydrology & Climate");
      ImGui::SliderFloat("Global Sea Level", &settings.seaLevel, 0.0f, 1.0f);
      if (ImGui::Button("Run Hydrology Cycle")) {
        TerrainController::GenerateClimate(buffers, settings);
        TerrainController::SimulateHydrology(buffers, settings);
      }
      ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("Sculpt")) {
      ImGui::Text("Brush Configuration");
      const char *modes[] = {"Raise Land", "Lower Land", "Smooth",
                             "Paint Biome"};
      ImGui::Combo("Tool Type", &brushMode, modes, 4);
      ImGui::SliderFloat("Brush Radius", &brushSize, 1.0f, 100.0f);
      ImGui::SliderFloat("Intensity", &brushStrength, 0.001f, 0.2f);
      ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("File System")) {
      ImGui::Text("Import/Export Operations");
      static char path[128] = "heightmap.png";
      ImGui::InputText("Source PNG", path, 128);
      if (ImGui::Button("Load External Heightmap")) {
        TerrainController::LoadHeightmapFromImage(buffers, std::string(path));
      }
      ImGui::Separator();
      if (ImGui::Button("Export S.A.G.A. World (.map)")) {
        // BinaryExporter::SaveWorld(buffers, "bin/data/world.map");
      }
      ImGui::EndTabItem();
    }

    ImGui::EndTabBar();
  }
  ImGui::End();
}

int main(int, char **) {
  if (!glfwInit())
    return 1;

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow *window = glfwCreateWindow(
      1600, 900, "S.A.G.A. Architect - World Builder", NULL, NULL);
  if (!window)
    return 1;

  glfwMakeContextCurrent(window);
  glewInit();

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  // io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 330");

  Setup();

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

    DrawViewport();
    DrawTools();

    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
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
