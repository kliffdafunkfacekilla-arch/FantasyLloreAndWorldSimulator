#include "../../include/Environment.hpp"
#include "../../include/PlatformUtils.hpp"
#include "../../include/Terrain.hpp"
#include "../../include/WorldEngine.hpp"
#include "../../include/stb_image.h"


// --- CRITICAL: Include GLEW before GLFW ---
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "../../deps/imgui/backends/imgui_impl_glfw.h"
#include "../../deps/imgui/backends/imgui_impl_opengl3.h"
#include "../../deps/imgui/imgui.h"


#include <algorithm>
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

// --- TEXTURE GENERATOR ---
struct Color {
  unsigned char r, g, b;
};
Color biomeColors[] = {
    {10, 60, 120},   // OCEAN (Deep Blue)
    {100, 100, 100}, // SCORCHED (Dark Grey)
    {160, 160, 160}, // BARE (Grey)
    {200, 220, 230}, // TUNDRA (White-ish)
    {255, 255, 255}, // SNOW (White)
    {210, 180, 140}, // TEMPERATE_DESERT (Sand)
    {180, 200, 150}, // SHRUBLAND (Pale Green)
    {100, 180, 50},  // GRASSLAND (Green)
    {50, 140, 50},   // TEMP_DECIDUOUS (Forest Green)
    {30, 100, 30},   // TEMP_RAIN_FOREST (Dark Green)
    {230, 210, 150}, // SUBTROPICAL_DESERT (Bright Sand)
    {60, 160, 60},   // TROPICAL_SEASONAL (Lush Green)
    {20, 90, 20}     // TROPICAL_RAIN_FOREST (Deep Jungle)
};

void UpdateMapTexture() {
  int w = 1000;
  int h = 1000;

  static std::vector<unsigned char> pixels(w * h * 3);

  for (int i = 0; i < (int)buffers.count; ++i) {
    unsigned char r, g, b;

    // 1. Get Biome Color
    int biome = buffers.biomeID[i];
    if (biome >= 0 && biome <= 12) {
      Color c = biomeColors[biome];
      r = c.r;
      g = c.g;
      b = c.b;
    } else {
      r = 255;
      g = 0;
      b = 255; // Error Pink
    }

    // 2. Blend with Height (Shadows/Depth)
    float h_val = buffers.height[i];
    float shade = 0.8f + (h_val * 0.4f); // Brighter at peaks
    r = (unsigned char)std::clamp((float)r * shade, 0.0f, 255.0f);
    g = (unsigned char)std::clamp((float)g * shade, 0.0f, 255.0f);
    b = (unsigned char)std::clamp((float)b * shade, 0.0f, 255.0f);

    // 3. Shoreline Highlight
    if (h_val > settings.seaLevel && h_val < settings.seaLevel + 0.02f) {
      r = std::min(255, r + 40);
      g = std::min(255, g + 40);
      b = std::min(255, b + 20); // Sand tint
    }

    pixels[i * 3 + 0] = r;
    pixels[i * 3 + 1] = g;
    pixels[i * 3 + 2] = b;
  }

  if (mapTextureID == 0) {
    glGenTextures(1, &mapTextureID);
    glBindTexture(GL_TEXTURE_2D, mapTextureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  }

  glBindTexture(GL_TEXTURE_2D, mapTextureID);
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

      if (ImGui::Button("Generate New World", ImVec2(-1, 30))) {
        settings.seed = seed;
        TerrainController::GenerateHeightmap(buffers, settings);
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
        // BinaryExporter::SaveWorld(buffers, "data/world.map");
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
  ImGui::StyleColorsDark();
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

    // ImGui::DockSpaceOverViewport(ImGui::GetMainViewport()); // Removed as it
    // caused errors

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
