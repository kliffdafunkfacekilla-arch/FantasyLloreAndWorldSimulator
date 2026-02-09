#include "../../deps/imgui/backends/imgui_impl_glfw.h"
#include "../../deps/imgui/backends/imgui_impl_opengl3.h"
#include "../../deps/imgui/imgui.h"
#include "../../include/AssetManager.hpp"
#include "../../include/BinaryExporter.hpp"
#include "../../include/WorldEngine.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>


namespace fs = std::filesystem;

// --- GLOBALS ---
WorldBuffers buffers;
GLuint mapTextureID = 0;

// Playback
int currentYear = 0;
int maxYear = 0;
bool isPlaying = false;
float playbackSpeed = 2.0f;
float accumulator = 0.0f;

// Camera
float zoom = 1.0f;
float camX = 0.5f, camY = 0.5f; // Center (0.0-1.0)

// --- TACTICAL RENDERER ---
void DrawTacticalView(ImDrawList *drawList, ImVec2 winPos, ImVec2 winSize) {
  int side = 1000;

  // 1. Calculate Visible Grid Area
  float viewW = 1.0f / zoom;
  float viewH = (viewW * winSize.y) / winSize.x;

  int startX = (int)((camX - viewW / 2) * side);
  int startY = (int)((camY - viewH / 2) * side);
  int endX = startX + (int)(viewW * side);
  int endY = startY + (int)(viewH * side);

  // Clamp
  startX = std::max(0, startX);
  startY = std::max(0, startY);
  endX = std::min(side, endX);
  endY = std::min(side, endY);

  // 2. Iterate Visible Cells
  for (int y = startY; y < endY; ++y) {
    for (int x = startX; x < endX; ++x) {
      int i = y * side + x;
      int agentID = buffers.agentID[i];

      // Skip empty land
      if (agentID == -1)
        continue;

      // Get Screen Position of this cell
      float uvX = (float)x / side;
      float uvY = (float)y / side;

      // Project to Screen
      float scrX =
          winPos.x + ((uvX - camX) * zoom * winSize.x) + (winSize.x * 0.5f);
      float scrY =
          winPos.y + ((uvY - camY) * zoom * winSize.x) + (winSize.y * 0.5f);
      float cellSize = (zoom * winSize.x) / side;

      // 3. RENDER SOLDIERS
      int soldierCount = (int)(buffers.agentStrength[i] / 50.0f);
      soldierCount = std::min(soldierCount, 10); // Max 10 dots per cell

      ImU32 color = IM_COL32(200, 200, 200, 255);

      for (const auto &def : AssetManager::agentRegistry) {
        if (def.id == agentID) {
          color = IM_COL32((int)(def.color[0] * 255), (int)(def.color[1] * 255),
                           (int)(def.color[2] * 255), 255);
          break;
        }
      }

      for (int k = 0; k < soldierCount; ++k) {
        float offX = (float)((i * k * 1321) % 100) / 100.0f * cellSize;
        float offY = (float)((i * k * 3121) % 100) / 100.0f * cellSize;

        float time = (float)ImGui::GetTime();
        offX += sin(time * 10 + k) * (cellSize * 0.1f);
        offY += cos(time * 10 + k) * (cellSize * 0.1f);

        drawList->AddCircleFilled(ImVec2(scrX + offX, scrY + offY),
                                  cellSize * 0.15f, color);
      }
    }
  }
}

// --- MAP RENDERER ---
void UpdateTexture() {
  int w = 1000;
  int h = 1000;
  static std::vector<unsigned char> pixels(w * h * 3);

  for (int i = 0; i < (int)buffers.count; ++i) {
    float hgt = buffers.height[i];
    int agID = buffers.agentID[i];

    unsigned char r, g, b;
    if (hgt < 0.4f) {
      r = 10;
      g = 10;
      b = 100;
    } else if (hgt < 0.45f) {
      r = 180;
      g = 170;
      b = 120;
    } else if (hgt > 0.8f) {
      r = 220;
      g = 220;
      b = 220;
    } else {
      r = 30;
      g = 80;
      b = 30;
    }

    if (agID != -1) {
      bool found = false;
      for (const auto &def : AssetManager::agentRegistry) {
        if (def.id == agID) {
          r = (unsigned char)(def.color[0] * 255);
          g = (unsigned char)(def.color[1] * 255);
          b = (unsigned char)(def.color[2] * 255);
          found = true;
          break;
        }
      }
      if (!found) {
        r = 200;
        g = 50;
        b = 50;
      }
    }
    pixels[i * 3] = r;
    pixels[i * 3 + 1] = g;
    pixels[i * 3 + 2] = b;
  }

  if (mapTextureID == 0)
    glGenTextures(1, &mapTextureID);
  glBindTexture(GL_TEXTURE_2D, mapTextureID);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE,
               pixels.data());
}

void LoadFrame(int year) {
  std::string path = "data/history/year_" + std::to_string(year) + ".map";
  if (BinaryExporter::LoadWorld(buffers, path)) {
    UpdateTexture();
  }
}

void DrawUI() {
  ImGui::Begin("Projector", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
  if (ImGui::SliderInt("Timeline", &currentYear, 0, maxYear))
    LoadFrame(currentYear);

  if (ImGui::Button(isPlaying ? "||" : ">"))
    isPlaying = !isPlaying;
  ImGui::SameLine();
  ImGui::SliderFloat("Speed", &playbackSpeed, 1.0f, 20.0f);

  ImGui::Separator();
  ImGui::Text("Zoom: %.2f (Scroll to Zoom)", zoom);
  if (zoom > 5.0f)
    ImGui::TextColored(ImVec4(1, 1, 0, 1), "TACTICAL VIEW ACTIVE");

  ImGui::End();
}

int main(int, char **) {
  if (!glfwInit())
    return 1;
  GLFWwindow *window =
      glfwCreateWindow(1280, 720, "S.A.G.A. Projector", NULL, NULL);
  glfwMakeContextCurrent(window);
  glewInit();
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 130");

  buffers.Initialize(1000000);
  AssetManager::Initialize();

  while (
      fs::exists("data/history/year_" + std::to_string(maxYear + 1) + ".map"))
    maxYear++;
  LoadFrame(0);

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    if (isPlaying && currentYear < maxYear) {
      accumulator += ImGui::GetIO().DeltaTime * playbackSpeed;
      if (accumulator >= 1.0f) {
        currentYear += (int)accumulator;
        if (currentYear > maxYear)
          currentYear = maxYear;
        accumulator = 0;
        LoadFrame(currentYear);
      }
    }

    ImGuiIO &io = ImGui::GetIO();
    if (!io.WantCaptureMouse) {
      if (io.MouseWheel != 0) {
        zoom += io.MouseWheel * 0.5f;
        if (zoom < 0.1f)
          zoom = 0.1f;
      }
      if (ImGui::IsMouseDragging(0)) {
        camX -= io.MouseDelta.x / (1000.0f * zoom);
        camY -= io.MouseDelta.y / (1000.0f * zoom);
      }
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    int w, h;
    glfwGetWindowSize(window, &w, &h);
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2((float)w, (float)h));
    ImGui::Begin("View", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground);

    if (zoom <= 5.0f) {
      float uvRange = 0.5f / zoom;
      ImGui::Image((void *)(intptr_t)mapTextureID, ImVec2((float)w, (float)h),
                   ImVec2(std::clamp(camX - uvRange, 0.0f, 1.0f - 2 * uvRange),
                          std::clamp(camY - uvRange, 0.0f, 1.0f - 2 * uvRange)),
                   ImVec2(std::clamp(camX + uvRange, 2 * uvRange, 1.0f),
                          std::clamp(camY + uvRange, 2 * uvRange, 1.0f)));
    } else {
      DrawTacticalView(ImGui::GetWindowDrawList(), ImGui::GetCursorScreenPos(),
                       ImVec2((float)w, (float)h));
    }
    ImGui::End();

    DrawUI();

    ImGui::Render();
    glViewport(0, 0, w, h);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window);
  }
  return 0;
}
