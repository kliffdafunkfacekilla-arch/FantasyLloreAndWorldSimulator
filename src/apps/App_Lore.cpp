#define GLFW_INCLUDE_NONE
#include "../../deps/imgui/backends/imgui_impl_glfw.h"
#include "../../deps/imgui/backends/imgui_impl_opengl3.h"
#include "../../deps/imgui/imgui.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>

// Modular Headers
#include "../../include/AssetManager.hpp"
#include "../../include/Lore.hpp"

// Prototypes (Defined in EditorUI.cpp)
void DrawDatabaseEditor(bool *p_open);

int main() {
  std::cout << "[LOG] Starting S.A.G.A. Database...\n";
  if (!glfwInit())
    return -1;

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  int windowW = 1200;
  int windowH = 800;
  GLFWwindow *window = glfwCreateWindow(
      windowW, windowH, "S.A.G.A. Database - Lore & Rules", NULL, NULL);
  if (!window)
    return -1;

  glfwMakeContextCurrent(window);
  glewInit();

  // 2. Initialize Data
  AssetManager::Initialize();
  LoreScribeNS::Initialize();

  // 3. Setup ImGui
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 330");
  ImGui::StyleColorsDark();

  bool showEditor = true;

  while (!glfwWindowShouldClose(window) && showEditor) {
    glfwPollEvents();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Re-use existing EditorUI logic
    DrawDatabaseEditor(&showEditor);

    // Standard Frame End
    glClearColor(0.1f, 0.1f, 0.12f, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window);
  }

  // Auto-save on exit
  AssetManager::SaveAll();

  // Cleanup
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  glfwDestroyWindow(window);
  glfwTerminate();

  std::cout << "[LOG] S.A.G.A. Database Session Closed. Rules Saved.\n";
  return 0;
}
