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

// --- GLOBALS ---
WorldBuffers buffers;
WorldSettings settings;

// Visuals
float zoom = 1.0f;
GLuint mapTextureID = 0; // The GPU Handle for our map image

// --- UI STATE ---
int brushMode = 0; // 0:Raise, 1:Lower, 2:Smooth
float brushSize = 20.0f;
float brushStrength = 0.5f;
bool mapDirty = true; // Flag to redraw texture

// --- TEXTURE GENERATOR (RELIEF MAPPER) ---

void UpdateMapTexture() {
  int w = 1000;
  int h = 1000;
  static std::vector<unsigned char> pixels(w * h * 3);

  // Light Direction (Simulated Sun from Top-Left)
  // float lightX = -1.0f;
  // float lightY = -1.0f;

  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      int i = y * w + x;
      float height = buffers.height[i];

      // --- 1. CALCULATE NORMAL (SLOPE) ---
      // Get height of neighbors (clamped to edges)
      float hL = (x > 0) ? buffers.height[i - 1] : height;
      float hR = (x < w - 1) ? buffers.height[i + 1] : height;
      float hU = (y > 0) ? buffers.height[i - w] : height;
      float hD = (y < h - 1) ? buffers.height[i + w] : height;

      // Simple gradient vector
      float dx = (hL - hR) * 20.0f; // Multiplier amplifies "steepness"
      float dy = (hU - hD) * 20.0f;
      float dz = 1.0f; // Up vector

      // Normalize
      float len = std::sqrt(dx * dx + dy * dy + dz * dz);
      dx /= len;
      dy /= len;
      dz /= len;

      // --- 2. CALCULATE LIGHTING ---
      // Dot Product of Slope vs Sun
      // Sun direction: 0.5, 0.5, 0.5 (Top Left)
      float light = (dx * 0.5f) + (dy * 0.5f) + (dz * 0.7f);

      // Contrast curve (make shadows darker)
      light = std::clamp(light, 0.4f, 1.1f);

      // --- 3. BASE COLOR (BIOMES) ---
      unsigned char r, g, b;

      // Water (Specially handled - no relief map, just depth)
      if (height < settings.seaLevel) {
        float depth = (settings.seaLevel - height) * 10.0f;
        r = 10;
        g = (unsigned char)std::clamp(80.0f - depth * 20.0f, 0.0f, 255.0f);
        b = (unsigned char)std::clamp(180.0f - depth * 50.0f, 0.0f, 255.0f);
        light = 1.0f; // Water is flat

        // Shoreline foam
        if (height > settings.seaLevel - 0.01f) {
          r = 200;
          g = 220;
          b = 255;
        }
      }
      // Sand
      else if (height < settings.seaLevel + 0.04f) {
        r = 210;
        g = 200;
        b = 130;
      }
      // Grass / Forest
      else if (height < 0.6f) {
        r = 60;
        g = 140;
        b = 60;
        // Moisture variation (if available)
        // if (buffers.moisture[i] < 0.3f) { r=160; g=160; b=80; } // Dry grass
      }
      // Rock
      else if (height < 0.85f) {
        r = 100;
        g = 90;
        b = 80;
      }
      // Snow
      else {
        r = 240;
        g = 240;
        b = 255;
      }

      // --- 4. APPLY LIGHTING ---
      pixels[i * 3 + 0] =
          (unsigned char)std::clamp((float)r * light, 0.0f, 255.0f);
      pixels[i * 3 + 1] =
          (unsigned char)std::clamp((float)g * light, 0.0f, 255.0f);
      pixels[i * 3 + 2] =
          (unsigned char)std::clamp((float)b * light, 0.0f, 255.0f);
    }
  }

  // Upload to GPU
  if (mapTextureID == 0)
    glGenTextures(1, &mapTextureID);
  glBindTexture(GL_TEXTURE_2D, mapTextureID);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR); // Linear for smoother look
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
                   ImGuiWindowFlags_NoScrollWithMouse);

  ImVec2 winSize = ImGui::GetContentRegionAvail();
  float dispW = winSize.x * zoom;
  float dispH = winSize.x * zoom; // Assuming square map 1000x1000

  ImVec2 cursorStart = ImGui::GetCursorScreenPos();

  if (mapTextureID != 0) {
    ImGui::Image((void *)(intptr_t)mapTextureID, ImVec2(dispW, dispH));
  }

  // Handle Brush Input
  if (ImGui::IsItemHovered() && ImGui::IsMouseDown(0)) {
    ImVec2 mPos = ImGui::GetMousePos();
    float relX = (mPos.x - cursorStart.x) / dispW;
    float relY = (mPos.y - cursorStart.y) / dispH;

    if (relX >= 0 && relX <= 1 && relY >= 0 && relY <= 1) {
      int gridX = (int)(relX * 1000);
      int gridY = (int)(relY * 1000);
      TerrainController::ApplyBrush(buffers, 1000, gridX, gridY, brushSize,
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
  ImGui::Begin("Architect Command Center");

  if (ImGui::BeginTabBar("Tabs")) {
    if (ImGui::BeginTabItem("Generation")) {
      ImGui::TextColored(ImVec4(1, 1, 0, 1), "Procedural Generation");
      static int seed = 12345;
      ImGui::InputInt("Seed", &seed);

      if (ImGui::Button("Generate New World (Perlin)", ImVec2(-1, 30))) {
        settings.seed = seed;
        TerrainController::GenerateHeightmap(buffers, settings);
        ClimateSim::Update(buffers, settings);
        mapDirty = true;
      }

      ImGui::Separator();
      ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "Advanced Tectonics");
      if (ImGui::Button("Generate Continents (Voronoi)", ImVec2(-1, 30))) {
        settings.seed = seed;
        TerrainController::GenerateTectonicPlates(buffers, settings);
        ClimateSim::Update(buffers, settings);
        mapDirty = true;
      }

      ImGui::Separator();
      ImGui::TextColored(ImVec4(0, 1, 1, 1), "Climate Control");

      if (ImGui::SliderFloat("Sea Level", &settings.seaLevel, 0.0f, 1.0f))
        mapDirty = true;
      if (ImGui::SliderFloat("Global Temp", &settings.globalTemp, -0.5f, 0.5f))
        mapDirty = true;
      if (ImGui::SliderAngle("Wind Dir", &settings.windAngle))
        mapDirty = true;
      if (ImGui::SliderFloat("Rainfall", &settings.raininess, 0.0f, 3.0f))
        mapDirty = true;

      if (ImGui::Button("Recalculate Climate")) {
        ClimateSim::Update(buffers, settings);
        mapDirty = true;
      }
      ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("Sculpt")) {
      ImGui::TextColored(ImVec4(0, 1, 1, 1), "Brush Tool");
      const char *modes[] = {"Raise", "Lower", "Flatten"};
      ImGui::Combo("Mode", &brushMode, modes, 3);
      ImGui::SliderFloat("Radius", &brushSize, 1.0f, 100.0f);
      ImGui::SliderFloat("Strength", &brushStrength, 0.01f, 1.0f);
      ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("File System")) {
      if (ImGui::Button("Save Map (data/world.map)")) {
        BinaryExporter::SaveWorld(buffers, "data/world.map");
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
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

  GLFWwindow *window =
      glfwCreateWindow(1600, 900, "S.A.G.A. Architect", NULL, NULL);
  if (!window)
    return 1;

  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

  if (glewInit() != GLEW_OK)
    return 1;

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
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
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
