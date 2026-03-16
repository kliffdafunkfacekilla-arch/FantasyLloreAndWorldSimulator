#include "../../deps/imgui/backends/imgui_impl_glfw.h"
#include "../../deps/imgui/backends/imgui_impl_opengl3.h"
#include "../../deps/imgui/imgui.h"
#include "../../include/AssetManager.hpp"
#include "../../include/BinaryExporter.hpp"
#include "../../include/SagaConfig.hpp"
#include "../../include/Theme.hpp"
#include "../../include/WorldEngine.hpp"
#include "../../include/nlohmann/json.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using json = nlohmann::json;

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

// Campaign Overlay
bool showCampaign = false;
json campaignData;

// --- HELPERS ---
ImVec2 GridToScreen(float x, float y, ImVec2 winPos, ImVec2 winSize) {
  float uvX = x / 1000.0f;
  float uvY = y / 1000.0f;
  float scrX =
      winPos.x + ((uvX - camX) * zoom * winSize.x) + (winSize.x * 0.5f);
  float scrY =
      winPos.y + ((uvY - camY) * zoom * winSize.x) + (winSize.y * 0.5f);
  return ImVec2(scrX, scrY);
}

// --- STRUCTURE RENDERER ---
void DrawStructure(ImDrawList *drawList, ImVec2 pos, float size, uint8_t type) {
  if (type == 0)
    return;

  if (type == 1) { // Road
    ImVec2 p1 = ImVec2(pos.x + size * 0.2f, pos.y + size * 0.2f);
    ImVec2 p2 = ImVec2(pos.x + size * 0.8f, pos.y + size * 0.8f);
    drawList->AddRectFilled(p1, p2, IM_COL32(100, 100, 100, 180)); // Grey stone
  } else if (type == 2) {                                          // Village
    // Square base
    ImVec2 b1 = ImVec2(pos.x + size * 0.2f, pos.y + size * 0.5f);
    ImVec2 b2 = ImVec2(pos.x + size * 0.8f, pos.y + size * 0.9f);
    drawList->AddRectFilled(b1, b2, IM_COL32(139, 69, 19, 255)); // Brown wood
    // Triangle roof
    ImVec2 t1 = ImVec2(pos.x + size * 0.15f, pos.y + size * 0.5f);
    ImVec2 t2 = ImVec2(pos.x + size * 0.85f, pos.y + size * 0.5f);
    ImVec2 t3 = ImVec2(pos.x + size * 0.5f, pos.y + size * 0.2f);
    drawList->AddTriangleFilled(t1, t2, t3, IM_COL32(160, 82, 45, 255));
  } else if (type == 3) { // City
    // Main block
    ImVec2 c1 = ImVec2(pos.x + size * 0.1f, pos.y + size * 0.3f);
    ImVec2 c2 = ImVec2(pos.x + size * 0.9f, pos.y + size * 0.9f);
    drawList->AddRectFilled(c1, c2, IM_COL32(120, 120, 120, 255)); // Stone grey
    // Crenellations
    float seg = size * 0.8f / 5.0f;
    for (int k = 0; k < 5; k += 2) {
      ImVec2 cr1 = ImVec2(c1.x + k * seg, pos.y + size * 0.15f);
      ImVec2 cr2 = ImVec2(c1.x + (k + 1) * seg, c1.y);
      drawList->AddRectFilled(cr1, cr2, IM_COL32(100, 100, 100, 255));
    }
  }
}

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
      uint8_t structType =
          buffers.structureType ? buffers.structureType[i] : (uint8_t)0;

      // Skip empty land
      if (agentID == -1 && structType == 0)
        continue;

      // Project to Screen
      float uvX = (float)x / side;
      float uvY = (float)y / side;
      float scrX =
          winPos.x + ((uvX - camX) * zoom * winSize.x) + (winSize.x * 0.5f);
      float scrY =
          winPos.y + ((uvY - camY) * zoom * winSize.x) + (winSize.y * 0.5f);
      float cellSize = (zoom * winSize.x) / side;

      // 3. RENDER STRUCTURES
      if (structType > 0) {
        DrawStructure(drawList, ImVec2(scrX, scrY), cellSize, structType);
      }

      // 4. RENDER AGENTS/ARMIES
      if (agentID != -1) {
        int soldierCount = (int)(buffers.agentStrength[i] / 50.0f);
        soldierCount =
            std::clamp(soldierCount, 1, 15); // Scale with zoom later?

        ImU32 color = IM_COL32(200, 200, 200, 255);
        for (const auto &def : AssetManager::agentRegistry) {
          if (def.id == agentID) {
            color =
                IM_COL32((int)(def.color[0] * 255), (int)(def.color[1] * 255),
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

          // Render as triangles (units) instead of circles
          ImVec2 p1 = ImVec2(scrX + offX, scrY + offY - cellSize * 0.1f);
          ImVec2 p2 = ImVec2(scrX + offX - cellSize * 0.08f,
                             scrY + offY + cellSize * 0.08f);
          ImVec2 p3 = ImVec2(scrX + offX + cellSize * 0.08f,
                             scrY + offY + cellSize * 0.08f);
          drawList->AddTriangleFilled(p1, p2, p3, color);
        }
      }

      // 5. DATA OVERLAYS (High Zoom)
      if (zoom > 15.0f && agentID != -1) {
        // Health Bar (Strength)
        float strengthPercent =
            std::clamp(buffers.agentStrength[i] / 1000.0f, 0.0f, 1.0f);
        ImVec2 barStart = ImVec2(scrX, scrY - cellSize * 0.2f);
        ImVec2 barEnd = ImVec2(scrX + cellSize, scrY - cellSize * 0.1f);
        drawList->AddRectFilled(barStart, barEnd, IM_COL32(50, 50, 50, 200));
        drawList->AddRectFilled(
            barStart, ImVec2(barStart.x + cellSize * strengthPercent, barEnd.y),
            IM_COL32(0, 255, 0, 255));

        if (zoom > 30.0f) {
          char coords[32];
          sprintf(coords, "%d,%d", x, y);
          drawList->AddText(NULL, cellSize * 0.3f,
                            ImVec2(scrX, scrY + cellSize),
                            IM_COL32(255, 255, 255, 150), coords);
        }
      }
    }
  }
}

// --- CAMPAIGN RENDERER ---
void DrawCampaignOverlay(ImDrawList *drawList, ImVec2 winPos, ImVec2 winSize) {
  if (campaignData.empty() || !campaignData.contains("nodes"))
    return;

  auto &nodes = campaignData["nodes"];
  ImU32 nodeColor = IM_COL32(255, 255, 0, 255);      // Yellow
  ImU32 lineColor = IM_COL32(255, 255, 0, 150);      // Faded Yellow
  ImU32 encounterColor = IM_COL32(255, 100, 0, 255); // Orange

  ImVec2 lastPos;
  bool first = true;

  for (auto &node : nodes) {
    float x = node["location"][0];
    float y = node["location"][1];
    ImVec2 p = GridToScreen(x, y, winPos, winSize);

    // Draw Line from previous node
    if (!first) {
      drawList->AddLine(lastPos, p, lineColor, 3.0f);
    }

    // Draw Node Icon
    drawList->AddCircleFilled(p, 8.0f, nodeColor);
    drawList->AddCircle(p, 10.0f, nodeColor, 12, 2.0f);

    // Label
    std::string label = node["type"].get<std::string>();
    drawList->AddText(ImVec2(p.x + 12, p.y - 6), nodeColor, label.c_str());

    lastPos = p;
    first = false;
  }

  // Draw travel encounters as points along the path (simplified visualization)
  if (campaignData.contains("travel_encounters") && nodes.size() >= 2) {
    auto &encounters = campaignData["travel_encounters"];
    float numE = (float)encounters.size();
    if (numE > 0) {
      ImVec2 p1 = GridToScreen(nodes[0]["location"][0], nodes[0]["location"][1],
                               winPos, winSize);
      ImVec2 p2 = GridToScreen(nodes.back()["location"][0],
                               nodes.back()["location"][1], winPos, winSize);

      for (int i = 0; i < (int)numE; ++i) {
        float lerp = (float)(i + 1) / (numE + 1.0f);
        ImVec2 ep =
            ImVec2(p1.x + (p2.x - p1.x) * lerp, p1.y + (p2.y - p1.y) * lerp);

        drawList->AddCircleFilled(ep, 4.0f, encounterColor);
        if (ImGui::IsMouseHoveringRect(ImVec2(ep.x - 5, ep.y - 5),
                                       ImVec2(ep.x + 5, ep.y + 5))) {
          std::string encTitle = encounters[i]["title"].get<std::string>();
          ImGui::SetTooltip("%s", encTitle.c_str());
        }
      }
    }
  }
}

// --- MAP RENDERER (RELIEF MAPPER) ---
void UpdateTexture() {
  int w = 1000;
  int h = 1000;
  static std::vector<unsigned char> pixels(w * h * 3);
  float seaLevel = 0.4f; // Default sea level for Replay

  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      int i = y * w + x;
      float height = buffers.height[i];
      int agID = buffers.agentID[i];

      // --- 1. CALCULATE NORMAL (SLOPE) ---
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

      // --- 2. CALCULATE LIGHTING ---
      float light = (dx * 0.5f) + (dy * 0.5f) + (dz * 0.7f);
      light = std::clamp(light, 0.4f, 1.1f);

      // --- 3. BASE COLOR ---
      unsigned char r, g, b;

      if (height < seaLevel) {
        float depth = (seaLevel - height) * 10.0f;
        r = 10;
        g = (unsigned char)std::clamp(80.0f - depth * 20.0f, 0.0f, 255.0f);
        b = (unsigned char)std::clamp(180.0f - depth * 50.0f, 0.0f, 255.0f);
        light = 1.0f;
        if (height > seaLevel - 0.01f) {
          r = 200;
          g = 220;
          b = 255;
        }
      } else if (height < seaLevel + 0.04f) {
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

      // Agent Color Overlay
      if (agID != -1) {
        for (const auto &def : AssetManager::agentRegistry) {
          if (def.id == agID) {
            r = (unsigned char)(def.color[0] * 255);
            g = (unsigned char)(def.color[1] * 255);
            b = (unsigned char)(def.color[2] * 255);
            break;
          }
        }
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

  if (mapTextureID == 0)
    glGenTextures(1, &mapTextureID);
  glBindTexture(GL_TEXTURE_2D, mapTextureID);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE,
               pixels.data());
}

void LoadFrame(int year) {
  std::string path =
      SagaConfig::DATA_HUB + "history/year_" + std::to_string(year) + ".map";
  if (BinaryExporter::LoadSnapshot(buffers, path)) {
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

  ImGui::Separator();
  if (ImGui::Checkbox("Show Campaign Overlay", &showCampaign)) {
    if (showCampaign) {
      std::ifstream f(SagaConfig::DATA_HUB + "current_campaign.json");
      if (f.is_open()) {
        f >> campaignData;
        std::cout << "[LOG] Loaded campaign data.\n";
      } else {
        std::cout << "[ERROR] Could not find " << SagaConfig::DATA_HUB
                  << "current_campaign.json\n";
        showCampaign = false;
      }
    }
  }

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
  SetupSAGATheme();
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 130");

  buffers.Initialize(1000000);
  AssetManager::Initialize();

  // Load static terrain/climate basis first
  std::string terrainPath = SagaConfig::DATA_HUB + "history/terrain.map";
  if (!BinaryExporter::LoadWorld(buffers, terrainPath)) {
      std::cout << "[WARN] Could not find history/terrain.map, visual layers may be empty.\n";
  }

  while (fs::exists(SagaConfig::DATA_HUB + "history/year_" +
                    std::to_string(maxYear + 1) + ".map"))
    maxYear++;
  
  if (maxYear > 0) LoadFrame(5); // Load first sample
  else LoadFrame(0);

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
        zoom += io.MouseWheel * 1.5f;
        if (zoom < 0.1f)
          zoom = 0.1f;
        if (zoom > 50.0f)
          zoom = 50.0f;
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

    ImVec2 winPos = ImGui::GetCursorScreenPos();
    ImVec2 winSize = ImVec2((float)w, (float)h);

    if (zoom <= 5.0f) {
      float uvRange = 0.5f / zoom;
      ImGui::Image((void *)(intptr_t)mapTextureID, winSize,
                   ImVec2(std::clamp(camX - uvRange, 0.0f, 1.0f - 2 * uvRange),
                          std::clamp(camY - uvRange, 0.0f, 1.0f - 2 * uvRange)),
                   ImVec2(std::clamp(camX + uvRange, 2 * uvRange, 1.0f),
                          std::clamp(camY + uvRange, 2 * uvRange, 1.0f)));
    } else {
      DrawTacticalView(ImGui::GetWindowDrawList(), winPos, winSize);
    }

    // DRAW CAMPAIGN OVERLAY
    if (showCampaign) {
      DrawCampaignOverlay(ImGui::GetWindowDrawList(), winPos, winSize);
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
