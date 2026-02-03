#define GLEW_STATIC
#include "../include/MapRenderer.hpp"
#include "../include/MemoryManager.hpp"
#include "../include/PlatformUtils.hpp"
#include "../include/SimulationModules.hpp"
#include "../include/WorldEngine.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cstring> // For strncpy
#include <iostream>
#include <vector>


// --- Global Objects ---
WorldBuffers buffers;
WorldSettings settings;
ChronosConfig clockState;
LoreScribe scribe;
MapRenderer renderer;
SimulationLoop simulation;
NeighborGraph graph;
ClimateSim climate;
TerrainController terrainGen;
std::vector<AgentTemplate> registry;
std::vector<Faction> factions;

void RegenerateWorld() {
  std::cout << "[UI] Regenerating World...\n";
  terrainGen.GenerateTerrain(buffers, settings);
  climate.UpdateGlobalClimate(buffers, settings);
}

void UpdateClimateOnly() {
  std::cout << "[UI] Updating Climate...\n";
  climate.UpdateGlobalClimate(buffers, settings);
}

void DrawGodModeUI() {
  ImGui::Begin("God Mode Dashboard");

  if (ImGui::CollapsingHeader("1. World Structure & Height",
                              ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::InputInt("Seed", &settings.seed);
    if (ImGui::Button("Select Heightmap Image")) {
      std::string path = OpenFileDialog();
      if (!path.empty()) {
        strncpy(settings.heightmapPath, path.c_str(),
                sizeof(settings.heightmapPath) - 1);
        settings.heightmapPath[sizeof(settings.heightmapPath) - 1] = '\0';
        terrainGen.LoadFromImage(settings.heightmapPath, buffers);
      }
    }
    ImGui::Text("File: %s", settings.heightmapPath);

    ImGui::SliderFloat("Sea Level", &settings.seaLevel, 0.0f, 1.0f);
    ImGui::DragFloatRange2("Height Range", &settings.heightMin,
                           &settings.heightMax, 0.01f, 0.0f, 1.0f);
    ImGui::SliderFloat("Height Severity", &settings.severity, 0.1f, 5.0f);
    ImGui::SliderFloat("Noise Frequency", &settings.frequency, 0.001f, 0.1f);
    ImGui::SliderFloat("Clustering", &settings.featureClustering, 1.0f, 3.0f);

    if (ImGui::Button("Regenerate Terrain")) {
      RegenerateWorld();
    }
  }

  if (ImGui::CollapsingHeader("2. Climate & Wind Zones")) {
    const char *modes[] = {"Region", "North Hemisphere", "South Hemisphere",
                           "Whole World"};
    ImGui::Combo("Scale Mode", &settings.worldScaleMode, modes,
                 IM_ARRAYSIZE(modes));

    ImGui::SliderFloat("Global Temperature", &settings.globalTempModifier, 0.0f,
                       2.0f);
    ImGui::SliderFloat("Global Rainfall", &settings.rainfallModifier, 0.0f,
                       2.0f);
    ImGui::SliderFloat("Wind Strength", &settings.globalWindStrength, 0.0f,
                       2.0f);

    ImGui::Separator();
    ImGui::Checkbox("Manual Wind Override", &settings.manualWindOverride);
    if (settings.manualWindOverride) {
      ImGui::SliderFloat("North Pole Wind", &settings.windZoneWeights[0], -1.0f,
                         1.0f, "Dir: %.2f");
      ImGui::SliderFloat("N. Temperate Wind", &settings.windZoneWeights[1],
                         -1.0f, 1.0f, "Dir: %.2f");
      ImGui::SliderFloat("Equator Wind", &settings.windZoneWeights[2], -1.0f,
                         1.0f, "Dir: %.2f");
      ImGui::SliderFloat("S. Temperate Wind", &settings.windZoneWeights[3],
                         -1.0f, 1.0f, "Dir: %.2f");
      ImGui::SliderFloat("South Pole Wind", &settings.windZoneWeights[4], -1.0f,
                         1.0f, "Dir: %.2f");
    }

    if (ImGui::Button("Update Climate")) {
      UpdateClimateOnly();
    }
  }

  if (ImGui::CollapsingHeader("3. Thermal Erosion")) {
    ImGui::SliderInt("Erosion Passes", &settings.erosionIterations, 1, 100);
    if (ImGui::Button("Run Erosion Cycle")) {
      for (int i = 0; i < settings.erosionIterations; i++) {
        terrainGen.ApplyThermalErosion(buffers, graph);
      }
    }
  }

  if (ImGui::CollapsingHeader("Civilization & War")) {
    ImGui::SliderInt("Time Speed", &clockState.globalTimeScale, 0, 100);
    if (ImGui::Button("Spawn Random Faction")) {
      int randomCell = rand() % buffers.count;
      if (buffers.factionID)
        buffers.factionID[randomCell] = (rand() % 5) + 1;
      if (buffers.population)
        buffers.population[randomCell] = 100.0f;
    }
  }

  ImGui::End();
}

int main() {
  if (!glfwInit())
    return -1;

  GLFWwindow *window =
      glfwCreateWindow(1280, 720, "Omnis World Engine", NULL, NULL);
  if (!window) {
    glfwTerminate();
    return -1;
  }

  glfwMakeContextCurrent(window);
  if (glewInit() != GLEW_OK)
    return -1;

  // UI
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 330");
  ImGui::StyleColorsDark();

  // Init Logic
  MemoryManager mem;
  mem.InitializeWorld(settings, buffers);

  simulation.settings = settings;
  simulation.scribe.Initialize("OmnisWorld");

  // Initial Gen
  RegenerateWorld();

  NeighborFinder finder;
  finder.BuildGraph(buffers, settings.cellCount, graph);

  renderer.Setup(buffers);
  renderer.LoadShaders("shaders/world.vert", "shaders/world.frag");

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    DrawGodModeUI();

    simulation.Update(buffers, registry, factions, clockState, graph, 0.016f);

    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    renderer.Render(buffers);

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
  }

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  finder.Cleanup(graph);
  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}
