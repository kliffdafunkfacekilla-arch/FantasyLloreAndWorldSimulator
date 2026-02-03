#define GLEW_STATIC
#include "../include/MapRenderer.hpp"
#include "../include/MemoryManager.hpp"
#include "../include/SimulationModules.hpp"
#include "../include/WorldEngine.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
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

// Helper to fully regenerate world when "Regenerate" is clicked
void RegenerateWorld() {
  std::cout << "[UI] Regenerating World...\n";
  terrainGen.GenerateTerrain(buffers, settings);
  climate.UpdateGlobalClimate(buffers, settings);
  // Visuals updated next Render call automatically
}

void UpdateClimateOnly() {
  std::cout << "[UI] Updating Climate...\n";
  climate.UpdateGlobalClimate(buffers, settings);
  // Visuals updated next Render call automatically
}

void DrawGodModeUI() {
  ImGui::Begin("God Mode Dashboard");

  ImGui::Text("Calendar: Year %d, Month %d, Day %d", clockState.yearCount,
              clockState.monthCount, clockState.dayCount);

  // --- Environment / Generation ---
  if (ImGui::CollapsingHeader("World Generation",
                              ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::Text("Map Settings");
    ImGui::InputInt("Seed", &settings.seed);

    ImGui::Separator();
    ImGui::Text("Terrain Shape");
    ImGui::SliderFloat("Frequency", &settings.featureFrequency, 0.001f, 0.1f);
    ImGui::SliderFloat("Clustering", &settings.featureClustering, 1.0f, 3.0f);
    ImGui::SliderFloat("Severity", &settings.heightSeverity, 0.1f, 3.0f);
    ImGui::DragFloatRange2("Height Range", &settings.heightMin,
                           &settings.heightMax, 0.01f, 0.0f, 1.0f);

    if (ImGui::Button("Regenerate Terrain")) {
      RegenerateWorld();
    }
  }

  // --- Climate ---
  if (ImGui::CollapsingHeader("Climate & Atmosphere")) {
    const char *modes[] = {"Region", "North Hemisphere", "South Hemisphere",
                           "Whole World"};
    ImGui::Combo("Scale Mode", &settings.worldScaleMode, modes,
                 IM_ARRAYSIZE(modes));

    ImGui::SliderFloat("Global Temp", &settings.globalTempModifier, 0.0f, 2.0f);
    ImGui::SliderFloat("Rainfall", &settings.rainfallAmount, 0.0f, 3.0f);
    ImGui::SliderFloat("Wind Strength", &settings.globalWindStrength, 0.0f,
                       2.0f);
    ImGui::SliderFloat("Sea Level", &settings.seaLevel, 0.0f, 1.0f);

    if (ImGui::Button("Update Climate")) {
      UpdateClimateOnly();
    }
  }

  if (ImGui::CollapsingHeader("Civilization & War")) {
    ImGui::SliderFloat("Aggression", &settings.aggressionMult, 0.0f, 5.0f);
    ImGui::Checkbox("Pause Conflict", &settings.peaceMode);
    ImGui::SliderInt("Time Speed", &clockState.globalTimeScale, 0, 100);

    if (ImGui::Button("Spawn Random Faction")) {
      int randomCell = rand() % buffers.count;
      if (buffers.factionID)
        buffers.factionID[randomCell] = (rand() % 5) + 1;
      if (buffers.population)
        buffers.population[randomCell] = 100.0f;
      // Visuals updated next frame
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

  // Initial Gen
  simulation.settings = settings;
  simulation.scribe.Initialize("OmnisWorld");

  // Generate initial world
  RegenerateWorld();

  // Graph needs building once (unless map size changes, which is locked for
  // now)
  NeighborFinder finder;
  finder.BuildGraph(buffers, settings.cellCount, graph);

  renderer.Setup(buffers);
  renderer.LoadShaders("shaders/world.vert", "shaders/world.frag");

  std::vector<AgentTemplate> registry;
  std::vector<Faction> factions;

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    DrawGodModeUI();

    // Run Simulation Tick
    simulation.Update(buffers, registry, factions, clockState, graph, 0.016f);

    // --- RENDER ---
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

  // Cleanup
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  finder.Cleanup(graph);
  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}
