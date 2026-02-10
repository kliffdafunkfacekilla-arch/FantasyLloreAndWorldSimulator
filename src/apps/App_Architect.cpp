#include "../../include/BinaryExporter.hpp"
#include "../../include/Environment.hpp"
#include "../../include/PlatformUtils.hpp"
#include "../../include/Terrain.hpp"
#include "../../include/Theme.hpp"
#include "../../include/WorldEngine.hpp"
#include "../../include/stb_image.h"

// --- CRITICAL: Include GLEW before GLFW ---
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "../../deps/imgui/backends/imgui_impl_glfw.h"
#include "../../deps/imgui/backends/imgui_impl_opengl3.h"
#include "../../deps/imgui/imgui.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

// Helper clamp
template <typename T> T clamp_val(T val, T min, T max) {
  if (val < min)
    return min;
  if (val > max)
    return max;
  return val;
}

// --- GLOBALS ---
WorldBuffers buffers;
WorldSettings settings;

// Visuals
float zoom = 1.0f;
GLuint mapTextureID = 0;

// --- UI STATE ---
int brushMode = 0; // 0:Raise, 1:Lower, 2:Smooth
float brushSize = 20.0f;
float brushStrength = 0.5f;
bool mapDirty = true;

// --- TEXTURE GENERATOR (RELIEF MAPPER) ---

void UpdateMapTexture() {
  int w = 1000;
  int h = 1000;
  static std::vector<unsigned char> pixels(w * h * 3);

  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      int i = y * w + x;
      float height = buffers.height[i];

      float hL = (x > 0) ? buffers.height[i - 1] : height;
      float hR = (x < w - 1) ? buffers.height[i + 1] : height;
      float hU = (y > 0) ? buffers.height[i - w] : height;
      float hD = (y < h - 1) ? buffers.height[i + w] : height;

      float dx = (hL - hR) * 20.0f;
      float dy = (hU - hD) * 20.0f;
      float dz = 1.0f;

      float len = std::sqrt(dx * dx + dy * dy + dz * dz);
      dx /= len;
      dy /= len;
      dz /= len;

      float light = (dx * 0.5f) + (dy * 0.5f) + (dz * 0.7f);
      light = clamp_val(light, 0.4f, 1.1f);

      unsigned char r, g, b;

      if (height < settings.seaLevel) {
        float depth = (settings.seaLevel - height) * 10.0f;
        r = 10;
        g = (unsigned char)clamp_val(80.0f - depth * 20.0f, 0.0f, 255.0f);
        b = (unsigned char)clamp_val(180.0f - depth * 50.0f, 0.0f, 255.0f);
        light = 1.0f;
        if (height > settings.seaLevel - 0.01f) {
          r = 200;
          g = 220;
          b = 255;
        }
      } else if (height < settings.seaLevel + 0.04f) {
        r = 210;
        g = 200;
        b = 130;
      } else if (height < 0.6f) {
        r = 60;
        g = 140;
        b = 60;
      } else if (height < 0.85f) {
        r = 100;
        g = 90;
        b = 80;
      } else {
        r = 240;
        g = 240;
        b = 255;
      }

      pixels[i * 3 + 0] =
          (unsigned char)clamp_val((float)r * light, 0.0f, 255.0f);
      pixels[i * 3 + 1] =
          (unsigned char)clamp_val((float)g * light, 0.0f, 255.0f);
      pixels[i * 3 + 2] =
          (unsigned char)clamp_val((float)b * light, 0.0f, 255.0f);
    }
  }

  if (mapTextureID == 0)
    glGenTextures(1, &mapTextureID);
  glBindTexture(GL_TEXTURE_2D, mapTextureID);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE,
               pixels.data());
  mapDirty = false;
}

// --- INITIALIZATION ---
void Setup() {
  buffers.Initialize(1000 * 1000);
  TerrainController::GenerateHeightmap(buffers, settings);
  ClimateSim::Update(buffers, settings);
  UpdateMapTexture();
}

// --- MAP VIEW ---
void DrawViewport() {
  ImGui::Begin("World Viewport", nullptr,
               ImGuiWindowFlags_NoScrollbar |
                   ImGuiWindowFlags_NoScrollWithMouse |
                   ImGuiWindowFlags_NoMove);
  ImVec2 winSize = ImGui::GetContentRegionAvail();
  float dispW = winSize.x * zoom;
  float dispH =
      winSize.y * zoom; // Fix: Use y for height if possible, or maintain aspect

  // Maintain square aspect for the world map
  float size = std::min(winSize.x, winSize.y) * zoom;
  dispW = size;
  dispH = size;

  ImVec2 cursorStart = ImGui::GetCursorScreenPos();
  if (mapTextureID != 0) {
    ImGui::Image((void *)(intptr_t)mapTextureID, ImVec2(dispW, dispH));
  }

  if (ImGui::IsItemHovered() && ImGui::IsMouseDown(0)) {
    ImVec2 mPos = ImGui::GetMousePos();
    float relX = (mPos.x - cursorStart.x) / dispW;
    float relY = (mPos.y - cursorStart.y) / dispH;
    if (relX >= 0 && relX <= 1 && relY >= 0 && relY <= 1) {
      TerrainController::ApplyBrush(buffers, 1000, (int)(relX * 1000),
                                    (int)(relY * 1000), brushSize,
                                    brushStrength, brushMode);
      mapDirty = true;
    }
  }
  if (ImGui::IsItemHovered()) {
    float wheel = ImGui::GetIO().MouseWheel;
    if (wheel != 0) {
      zoom += wheel * 0.1f;
      if (zoom < 0.1f)
        zoom = 0.1f;
    }
  }
  ImGui::End();
}

// --- TOOLS PANEL ---
void DrawTools() {
  ImGui::Begin("Architect Command Center", nullptr,
               ImGuiWindowFlags_AlwaysAutoResize);

  if (ImGui::BeginTabBar("ArchitectTabs")) {
    // Landscape Tab: Primary map creation
    if (ImGui::BeginTabItem("Landscape")) {
      ImGui::SetNextItemWidth(150);
      ImGui::InputInt("Seed", &settings.seed);
      ImGui::SameLine();
      if (ImGui::Button("Randomize")) {
        settings.seed = rand();
        TerrainController::GenerateHeightmap(buffers, settings);
        mapDirty = true;
      }

      ImGui::Checkbox("Island Mode", &settings.islandMode);

      if (ImGui::Button("Generate New Heightmap", ImVec2(-1, 30))) {
        TerrainController::GenerateHeightmap(buffers, settings);
        mapDirty = true;
      }

      if (ImGui::Button("Generate Tectonic Plates", ImVec2(-1, 30))) {
        TerrainController::GenerateTectonicPlates(buffers, settings);
        mapDirty = true;
      }

      ImGui::Separator();
      ImGui::Text("Terraforming Tools");
      const char *modes[] = {"Raise", "Lower", "Smooth"};
      if (ImGui::Combo("Brush Mode", &brushMode, modes, 3)) {
      }

      ImGui::SliderFloat("Radius", &brushSize, 1.0f, 200.0f);
      ImGui::SliderFloat("Strength", &brushStrength, 0.01f, 1.0f);

      ImGui::Separator();
      if (ImGui::Button("Global Erosion")) {
        TerrainController::ApplyThermalErosion(buffers, 10);
        mapDirty = true;
      }
      ImGui::SameLine();
      if (ImGui::Button("Global Smooth")) {
        TerrainController::SmoothTerrain(buffers, 1000);
        mapDirty = true;
      }

      ImGui::EndTabItem();
    }

    // Climate Tab: Environmental settings
    if (ImGui::BeginTabItem("Climate")) {
      ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f),
                         "Atmospheric Controls");
      if (ImGui::SliderFloat("Sea Level", &settings.seaLevel, 0.0f, 1.0f))
        mapDirty = true;
      if (ImGui::SliderFloat("Rainfall", &settings.raininess, 0.0f, 3.0f))
        mapDirty = true;

      ImGui::Separator();
      ImGui::Text("Temperature Zones");
      if (ImGui::SliderFloat("Polar Zone", &settings.tempZonePolar, -1.0f,
                             1.0f))
        mapDirty = true;
      if (ImGui::SliderFloat("Temperate Zone", &settings.tempZoneTemperate,
                             -1.0f, 1.0f))
        mapDirty = true;
      if (ImGui::SliderFloat("Tropical Zone", &settings.tempZoneTropical, -1.0f,
                             1.0f))
        mapDirty = true;

      ImGui::Separator();
      ImGui::Text("Wind Currents (Prevailing Winds)");
      for (int i = 0; i < 5; ++i) {
        char label[64];
        sprintf(label, "Zone %d Direction", i);
        if (ImGui::SliderAngle(label, &settings.windZonesDir[i]))
          mapDirty = true;
        sprintf(label, "Zone %d Strength", i);
        if (ImGui::SliderFloat(label, &settings.windZonesStr[i], 0.0f, 2.0f))
          mapDirty = true;
      }
      ImGui::EndTabItem();
    }

    // IO Tab: File operations
    if (ImGui::BeginTabItem("Project")) {
      ImGui::Text("Heightmap Import/Export");
      static char manualPath[260] = "data/heightmap.png";
      ImGui::InputText("Import Path", manualPath, 260);
      if (ImGui::Button("Load External Heightmap")) {
        TerrainController::LoadHeightmapFromImage(buffers, manualPath);
        mapDirty = true;
      }

      ImGui::Separator();
      if (ImGui::Button("Save Current World (world.map)", ImVec2(-1, 40))) {
        BinaryExporter::SaveWorld(buffers, "data/world.map");
      }
      ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
  }

  if (mapDirty) {
    ImGui::TextColored(ImVec4(1, 1, 0, 1), "STATUS: RECALCULATING...");
  } else {
    ImGui::TextColored(ImVec4(0, 1, 0, 1), "STATUS: READY");
  }

  ImGui::End();
}

int main(int, char **) {
  if (!glfwInit())
    return 1;
  GLFWwindow *window =
      glfwCreateWindow(1600, 900, "S.A.G.A. Architect", NULL, NULL);
  if (!window)
    return 1;
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);
  glewInit();
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  SetupSAGATheme();
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 130");
  Setup();
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    if (mapDirty) {
      ClimateSim::Update(buffers, settings);
      UpdateMapTexture();
    }
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    DrawViewport();
    DrawTools();
    ImGui::Render();
    int dw, dh;
    glfwGetFramebufferSize(window, &dw, &dh);
    glViewport(0, 0, dw, dh);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window);
  }
  return 0;
}
