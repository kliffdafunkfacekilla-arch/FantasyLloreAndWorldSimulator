#include "GuiController.hpp"
#include "AssetManager.hpp"
#include "Biology.hpp"
#include "Environment.hpp"
#include <algorithm>
#include <cstdio>

// State for the GUI (moved from globals to static inside DrawMainLayout or
// passed refs)
static int s_activeTab = 0;
static bool s_showDBEditor = false;
static int s_paintMode = 0;
static float s_brushSize = 25.0f;
static float s_brushSpeed = 0.01f;
static bool s_isPaused = true;
static int s_viewMode = 0;

static ImGuiWindowFlags GetPanelFlags() {
  return ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
         ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus |
         ImGuiWindowFlags_NoTitleBar;
}

void GuiController::Initialize() {}

GuiState GuiController::DrawMainLayout(WorldBuffers &buffers,
                                       WorldSettings &settings,
                                       TerrainController &terrain,
                                       NeighborGraph &graph, int screenW,
                                       int screenH) {
  GuiState state;
  state.screenWidth = screenW;
  state.screenHeight = screenH;
  state.topBarHeight = 60;
  state.bottomPanelHeight = 200;
  state.leftPanelWidth = 350;
  state.rightPanelWidth = 350;

  // Layout Calculation
  state.viewX = state.leftPanelWidth;
  state.viewY = state.topBarHeight;
  state.viewW = screenW - state.leftPanelWidth - state.rightPanelWidth;
  state.viewH = screenH - state.topBarHeight - state.bottomPanelHeight;

  // 1. Toolbar (Top)
  DrawToolbar(state, settings, s_isPaused, s_viewMode);

  // 2. Architect Panel (Left)
  DrawArchitectPanel(state, buffers, settings, terrain, graph, s_activeTab,
                     s_paintMode, s_brushSize, s_brushSpeed);

  // Mouse Logic for Hovered Index & Painting
  static int s_hoveredIndex = -1;
  ImVec2 mousePos = ImGui::GetMousePos();
  if (mousePos.x >= state.viewX && mousePos.x < state.viewX + state.viewW &&
      mousePos.y >= state.viewY && mousePos.y < state.viewY + state.viewH) {

    float relativeY = mousePos.y - state.viewY;
    float normX = (mousePos.x - state.viewX) / (float)state.viewW;
    float normY = 1.0f - (relativeY / (float)state.viewH);

    int mapX = (int)(normX * 1000);
    int mapY = (int)(normY * 1000);

    if (mapX >= 0 && mapX < 1000 && mapY >= 0 && mapY < 1000) {
      s_hoveredIndex = mapY * 1000 + mapX;
    } else {
      s_hoveredIndex = -1;
    }
  } else {
    s_hoveredIndex = -1;
  }

  // Paint Logic Check
  if (s_activeTab == 0 && ImGui::IsMouseDown(ImGuiMouseButton_Left) &&
      s_hoveredIndex != -1) {

    // Calculate map coordinates
    int width = 1000; // Assumption
    int cx = s_hoveredIndex % width;
    int cy = s_hoveredIndex / width;

    terrain.ApplyBrush(buffers, width, cx, cy, s_brushSize, s_brushSpeed,
                       s_paintMode);
  }

  // 3. Inspector (Right)
  DrawInspector(state, buffers, s_hoveredIndex);

  // 4. Chronicle (Bottom)
  DrawChronicle(state);

  // 5. DB Editor (Popup)
  if (s_showDBEditor)
    DrawDatabaseEditor(&s_showDBEditor);

  // Fill Output State
  state.isPaused = s_isPaused;
  state.activeTab = s_activeTab;
  state.viewMode = s_viewMode;

  return state;
}

void GuiController::DrawToolbar(const GuiState &dim, WorldSettings &settings,
                                bool &isPaused, int &viewMode) {
  ImGui::SetNextWindowPos(ImVec2(0, 0));
  ImGui::SetNextWindowSize(
      ImVec2((float)dim.screenWidth, (float)dim.topBarHeight));

  ImGui::Begin("Toolbar", nullptr, GetPanelFlags());

  // Playback
  if (ImGui::Button("<<"))
    settings.timeScale = std::max(1, settings.timeScale - 1);
  ImGui::SameLine();
  if (ImGui::Button(isPaused ? "PLAY" : "PAUSE", ImVec2(60, 0)))
    isPaused = !isPaused;
  ImGui::SameLine();
  if (ImGui::Button(">>"))
    settings.timeScale = std::min(50, settings.timeScale + 1);
  ImGui::SameLine();
  ImGui::Text("Speed: %dx", settings.timeScale);

  ImGui::SameLine(300);
  // View Modes
  if (ImGui::RadioButton("Satellite", viewMode == 0))
    viewMode = 0;
  ImGui::SameLine();
  if (ImGui::RadioButton("Political", viewMode == 1))
    viewMode = 1;
  ImGui::SameLine();
  if (ImGui::RadioButton("Economic", viewMode == 2))
    viewMode = 2;
  ImGui::SameLine();
  if (ImGui::RadioButton("Magic", viewMode == 3))
    viewMode = 3;

  ImGui::SameLine(dim.screenWidth - 250);
  if (ImGui::Button("EDIT RULES / JSON"))
    s_showDBEditor = !s_showDBEditor;

  ImGui::End();
}

void GuiController::DrawArchitectPanel(
    const GuiState &dim, WorldBuffers &buffers, WorldSettings &settings,
    TerrainController &terrain, NeighborGraph &graph, int &activeTab,
    int &paintMode, float &brushSize, float &brushSpeed) {

  // UI State Variables (static for now as requested)
  static bool forceEdges = false;
  static float edgeFadeDist = 50.0f;

  ImGui::SetNextWindowPos(ImVec2(0, (float)dim.topBarHeight));
  ImGui::SetNextWindowSize(ImVec2(
      (float)dim.leftPanelWidth,
      (float)(dim.screenHeight - dim.topBarHeight - dim.bottomPanelHeight)));

  ImGui::Begin("Architect", nullptr, GetPanelFlags());

  if (ImGui::BeginTabBar("ArchTabs")) {
    // TAB A: Geo-Physics (Updated)
    if (ImGui::BeginTabItem("Geo-Physics")) {
      activeTab = 0;

      ImGui::TextColored(ImVec4(1, 1, 0, 1), "Map Generation");
      ImGui::InputInt("Seed", &settings.seed);
      if (ImGui::Button("Regenerate Full World", ImVec2(-1, 0))) {
        terrain.GenerateProceduralTerrain(buffers, settings);
        if (settings.erosionIterations > 0)
          terrain.ApplyThermalErosion(buffers, settings.erosionIterations);
        // Re-run checking logic immediately after gen
        if (forceEdges)
          terrain.EnforceOceanEdges(buffers, 1000, edgeFadeDist);

        for (int i = 0; i < 100; ++i)
          HydrologySim::Update(buffers, graph);
      }

      ImGui::SeparatorText("Global Modifiers");
      ImGui::SliderFloat("Sea Level", &settings.seaLevel, -1.0f, 1.0f);

      if (ImGui::Checkbox("Ocean Borders", &forceEdges)) {
        if (forceEdges)
          terrain.EnforceOceanEdges(buffers, 1000, edgeFadeDist);
      }
      if (forceEdges) {
        ImGui::Indent();
        ImGui::SliderFloat("Fade Dist", &edgeFadeDist, 10.0f, 200.0f);
        ImGui::Unindent();
      }

      if (ImGui::Button("Smooth Map"))
        terrain.SmoothTerrain(buffers, 1000);
      ImGui::SameLine();
      if (ImGui::Button("Roughen Coast"))
        terrain.RoughenCoastlines(buffers, 1000, settings.seaLevel);

      ImGui::SeparatorText("Erosion");
      if (ImGui::Button("Erode (1 Step)"))
        terrain.ApplyThermalErosion(buffers, 1);
      ImGui::SameLine();
      if (ImGui::Button("Erode (100)"))
        terrain.ApplyThermalErosion(buffers, 100);

      ImGui::Separator();
      ImGui::TextColored(ImVec4(0, 1, 1, 1), "Brush Tool");
      const char *modes[] = {"Raise", "Lower", "Flatten", "Noise"};
      ImGui::Combo("Mode", &paintMode, modes, 4);
      ImGui::SliderFloat("Size", &brushSize, 1.0f, 50.0f);
      ImGui::SliderFloat("Strength", &brushSpeed, 0.01f, 2.0f);
      ImGui::TextDisabled("(Click on Map to Apply)");

      ImGui::EndTabItem();
    }

    // TAB B: Life & Civ
    if (ImGui::BeginTabItem("Life & Civ")) {
      activeTab = 1; // Simulation Active

      ImGui::SeparatorText("Spawning");
      if (ImGui::Button("Spawn Civilizations")) {
        for (int i = 1; i <= 5; ++i)
          AgentSystem::SpawnCivilization(buffers, i);
      }
      if (ImGui::Button("Clear Biology")) {
        std::fill_n(buffers.population, buffers.count, 0);
        std::fill_n(buffers.factionID, buffers.count, 0);
      }

      ImGui::SeparatorText("Simulation Control");
      ImGui::Checkbox("Simulate Vegetation", &settings.enableBiology);
      ImGui::Checkbox("Enable Factions", &settings.enableFactions);
      ImGui::Checkbox("Enable War", &settings.enableConflict);

      ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
  }

  ImGui::End();
}

void GuiController::DrawInspector(const GuiState &dim, WorldBuffers &buffers,
                                  int hoveredIndex) {
  ImGui::SetNextWindowPos(ImVec2((float)(dim.screenWidth - dim.rightPanelWidth),
                                 (float)dim.topBarHeight));
  ImGui::SetNextWindowSize(ImVec2(
      (float)dim.rightPanelWidth,
      (float)(dim.screenHeight - dim.topBarHeight - dim.bottomPanelHeight)));

  ImGui::Begin("Inspector", nullptr, GetPanelFlags());

  if (hoveredIndex != -1 && hoveredIndex < (int)buffers.count) {
    ImGui::SeparatorText("Cell Data");
    ImGui::Text("ID: #%d", hoveredIndex);
    int y = hoveredIndex / 1000;
    int x = hoveredIndex % 1000;
    ImGui::Text("Coords: (%d, %d)", x, y);

    ImGui::SeparatorText("Environment");
    ImGui::Text("Height: %.2f", buffers.height[hoveredIndex]);
    ImGui::ProgressBar(buffers.height[hoveredIndex], ImVec2(-1, 0));

    ImGui::Text("Temp: %.0f C",
                buffers.temperature[hoveredIndex] * 50.0f - 10.0f);
    ImGui::Text("Rain: %.0f mm", buffers.moisture[hoveredIndex] * 2000.0f);

    ImGui::SeparatorText("Occupant");
    if (buffers.population[hoveredIndex] > 0) {
      ImGui::TextColored(ImVec4(0, 1, 0, 1), "Inhabited");
      ImGui::Text("Pop: %d", buffers.population[hoveredIndex]);
      ImGui::Text("Faction: %d", buffers.factionID[hoveredIndex]);
      if (buffers.civTier)
        ImGui::Text("Tier: %d", buffers.civTier[hoveredIndex]);
    } else {
      ImGui::TextDisabled("Wilderness");
    }

  } else {
    ImGui::TextDisabled("Hover over the map to inspect.");
  }

  ImGui::End();
}

void GuiController::DrawChronicle(const GuiState &dim) {
  ImGui::SetNextWindowPos(
      ImVec2((float)dim.leftPanelWidth,
             (float)(dim.screenHeight - dim.bottomPanelHeight)));
  ImGui::SetNextWindowSize(ImVec2(
      (float)(dim.screenWidth - dim.leftPanelWidth - dim.rightPanelWidth),
      (float)dim.bottomPanelHeight));

  ImGui::Begin("Chronicle", nullptr, GetPanelFlags());

  static bool showWars = true;
  static bool showFoundings = true;
  ImGui::Checkbox("Wars", &showWars);
  ImGui::SameLine();
  ImGui::Checkbox("Foundings", &showFoundings);

  ImGui::Separator();
  ImGui::BeginChild("LogScroll");
  ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
                     "[Year 100] Simulation Started.");
  ImGui::EndChild();
  ImGui::End();
}

void GuiController::DrawDatabaseEditor(bool *p_open) {
  ImGui::OpenPopup("Database Editor");
  if (ImGui::BeginPopupModal("Database Editor", p_open,
                             ImGuiWindowFlags_AlwaysAutoResize)) {
    if (ImGui::BeginTabBar("DBTabs")) {
      if (ImGui::BeginTabItem("Resources")) {
        for (auto &r : AssetManager::resourceRegistry) {
          ImGui::Text("%s ($%.2f)", r.name.c_str(), r.value);
        }
        ImGui::EndTabItem();
      }
      ImGui::EndTabBar();
    }
    if (ImGui::Button("Close"))
      *p_open = false;
    ImGui::EndPopup();
  }
}
