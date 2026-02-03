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

// --- Global Engine Objects ---
WorldBuffers buffers;
WorldSettings settings;
ChronosConfig clockState;
LoreScribe scribe;
MapRenderer renderer;
SimulationLoop simulation;
NeighborGraph graph;

void DrawGodModeUI() {
  ImGui::Begin("God Mode Dashboard");

  ImGui::Text("Calendar: Year %d, Month %d, Day %d", clockState.yearCount,
              clockState.monthCount, clockState.dayCount);
  ImGui::Separator();

  ImGui::SliderFloat("Sea Level", &settings.seaLevel, 0.0f, 1.0f);
  ImGui::SliderFloat("Global Temp", &settings.globalTempModifier, 0.0f, 2.0f);
  ImGui::SliderInt("Time Speed", &clockState.globalTimeScale, 0, 100);

  if (ImGui::Button("Spawn Random Faction")) {
    // Drop a faction seed at a random location
    int randomCell = rand() % buffers.count;
    if (buffers.factionID)
      buffers.factionID[randomCell] = (rand() % 5) + 1;
    if (buffers.population)
      buffers.population[randomCell] = 100.0f;
  }

  ImGui::End();
}

int main() {
  // 1. Initialize GLFW/OpenGL Window
  if (!glfwInit())
    return -1;

  // Decent resolution
  GLFWwindow *window =
      glfwCreateWindow(1280, 720, "Omnis World Engine", NULL, NULL);
  if (!window) {
    glfwTerminate();
    return -1;
  }

  glfwMakeContextCurrent(window);
  if (glewInit() != GLEW_OK)
    return -1;

  // 2. Initialize ImGui (The "Hands")
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 330");
  ImGui::StyleColorsDark();

  // 3. Setup World Data (The "Brain")
  std::cout << "[CORE] Allocating memory..." << std::endl;
  MemoryManager mem;
  mem.InitializeWorld(settings,
                      buffers); // Allocates 1,000,000 cells by default setting

  // Generation
  std::cout << "[CORE] Generating Terrain..." << std::endl;
  // We need to call the terrain generation logic.
  // Since we refactored, we can call the module functions directly or via
  // SimulationLoop if integrated. For now, let's assume SimulationLoop has a
  // 'Init' or we do it manually like before. Re-using the manual calls from
  // previous main for generation phase:

  // Quick local generation to ensure we have data
  // (Note: ideally this should be a function in SimulationLoop, but for now we
  // put it here or call a helper) We will use the simulation loop structure if
  // possible, but for immediate fix:
  simulation.settings = settings;
  simulation.scribe.Initialize("OmnisWorld");

  // Just run one Update to prompt generation/setup if needed?
  // actually we need the initial terrain generation function.
  // Let's rely on the simulation loop or re-implement the generator call.
  // The user's previous code had GenerateTerrain() function *in* main.cpp.
  // I should probably preserve that or move it to a module.
  // For now, I will instantiate TerrainController to generate.
  TerrainController terrainGen;
  terrainGen.GenerateTerrain(buffers, settings);

  NeighborFinder finder;
  finder.BuildGraph(buffers, settings.cellCount, graph);

  ClimateSim climate;
  climate.UpdateGlobalClimate(buffers, settings);

  // Renderer
  renderer.Initialize(buffers);
  renderer.LoadShaders("shaders/world.vert", "shaders/world.frag");

  // 4. THE MAIN LOOP
  std::vector<AgentTemplate> registry; // Empty for now
  std::vector<Faction> factions;       // Empty for now

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    // Start UI Frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    DrawGodModeUI();

    // Run Simulation Tick
    // We need to pass the graph we built
    simulation.Update(buffers, registry, factions, clockState, graph, 0.016f);

    // --- THE FIX FOR THE BLACK SCREEN ---
    renderer.UpdateVisuals(buffers, settings); // Pushes CPU data to GPU

    // Render
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    renderer.Render(); // Actually draws the points

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
