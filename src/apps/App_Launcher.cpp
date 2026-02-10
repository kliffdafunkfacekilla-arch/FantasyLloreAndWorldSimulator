#include "../../deps/imgui/backends/imgui_impl_glfw.h"
#include "../../deps/imgui/backends/imgui_impl_opengl3.h"
#include "../../deps/imgui/imgui.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <windows.h> // For ShellExecute

// --- THEME ---
void SetupLauncherTheme() {
  ImGuiStyle &style = ImGui::GetStyle();
  style.WindowRounding = 8.0f;
  style.FrameRounding = 5.0f;
  style.Colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
  style.Colors[ImGuiCol_Button] = ImVec4(0.20f, 0.20f, 0.25f, 1.00f);
  style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.30f, 0.30f, 0.35f, 1.00f);
  style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.15f, 0.15f, 0.20f, 1.00f);
  style.Colors[ImGuiCol_Text] = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);
}

// --- LAUNCHER HELPER ---
void RunApp(const char *exeName, const char *windowTitle) {
  // Check if already running to prevent "opens 3 times" bugs
  HWND hwnd = FindWindowA(NULL, windowTitle);
  if (hwnd) {
    SetForegroundWindow(hwnd);
    return;
  }

  // ShellExecute allows launching without freezing this app
  ShellExecuteA(NULL, "open", exeName, NULL, "bin", SW_SHOW);
}

void DrawDashboard() {
  // Center the window content
  ImGui::SetNextWindowPos(ImVec2(0, 0));
  ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
  ImGui::Begin("Launcher", nullptr,
               ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground);

  // Header
  ImGui::SetWindowFontScale(2.0f);
  float textW = ImGui::CalcTextSize("S.A.G.A. SUITE").x;
  ImGui::SetCursorPosX((ImGui::GetWindowWidth() - textW) * 0.5f);
  ImGui::SetCursorPosY(50);
  ImGui::TextColored(ImVec4(1, 0.8f, 0.4f, 1), "S.A.G.A. SUITE");
  ImGui::SetWindowFontScale(1.0f);

  float textW2 =
      ImGui::CalcTextSize("Simulated Anthropology & Geological Automaton").x;
  ImGui::SetCursorPosX((ImGui::GetWindowWidth() - textW2) * 0.5f);
  ImGui::TextDisabled("Simulated Anthropology & Geological Automaton");

  ImGui::Spacing();
  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();
  ImGui::Spacing();

  // Buttons Grid
  float btnW = 200;
  float btnH = 120;
  float startX = (ImGui::GetWindowWidth() - (btnW * 4 + 60)) * 0.5f;

  ImGui::SetCursorPosX(startX);

  // 1. ARCHITECT
  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.4f, 0.3f, 1));
  if (ImGui::Button("ARCHITECT\n\nBuild World\nPaint Terrain",
                    ImVec2(btnW, btnH))) {
    RunApp("SAGA_Architect.exe", "S.A.G.A. Architect");
  }
  ImGui::PopStyleColor();

  ImGui::SameLine();

  // 2. DATABASE
  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.3f, 0.4f, 1));
  if (ImGui::Button("DATABASE\n\nWrite Lore\nDefine Rules\nCalendar",
                    ImVec2(btnW, btnH))) {
    RunApp("SAGA_Database.exe", "S.A.G.A. Database");
  }
  ImGui::PopStyleColor();

  ImGui::SameLine();

  // 3. ENGINE
  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.2f, 0.2f, 1));
  if (ImGui::Button("ENGINE\n\nRun Simulation\nGenerate History",
                    ImVec2(btnW, btnH))) {
    RunApp("SAGA_Engine.exe", "S.A.G.A. Engine");
  }
  ImGui::PopStyleColor();

  ImGui::SameLine();

  // 4. PROJECTOR
  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.4f, 0.2f, 1));
  if (ImGui::Button("PROJECTOR\n\nWatch Replay\nPhoto Mode",
                    ImVec2(btnW, btnH))) {
    RunApp("SAGA_Projector.exe", "S.A.G.A. Projector");
  }
  ImGui::PopStyleColor();

  // Footer
  ImGui::SetCursorPosY(ImGui::GetWindowHeight() - 50);
  ImGui::Separator();
  ImGui::TextDisabled("v1.0.0 | Output Directory: /data/");
  ImGui::SameLine();
  float exitW = ImGui::CalcTextSize("EXIT").x + 40;
  ImGui::SetCursorPosX(ImGui::GetWindowWidth() - exitW);
  if (ImGui::Button("EXIT", ImVec2(-1, 0)))
    exit(0);

  ImGui::End();
}

int main(int, char **) {
  if (!glfwInit())
    return 1;
  // Transparent / Borderless window feeling
  glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);

  GLFWwindow *window = glfwCreateWindow(900, 400, "SAGA Launcher", NULL, NULL);
  if (!window)
    return 1;
  glfwMakeContextCurrent(window);
  glewInit();

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  SetupLauncherTheme();
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 130");

  // Center window on screen
  const GLFWvidmode *mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
  glfwSetWindowPos(window, (mode->width - 900) / 2, (mode->height - 400) / 2);

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    DrawDashboard();

    ImGui::Render();
    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    glViewport(0, 0, w, h);
    glClearColor(0, 0, 0, 0);
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
