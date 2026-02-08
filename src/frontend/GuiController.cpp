#include "GuiController.hpp"
#include "../../include/AssetManager.hpp"
#include "../../include/Biology.hpp"
#include "../../include/Environment.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

// State for the GUI (moved from globals to static inside DrawMainLayout or
// passed refs)
static int s_activeTab = 0;
static bool s_showDBEditor = false;
static int s_paintMode = 0;
static float s_brushSize = 25.0f;
static float s_brushSpeed = 0.01f;
static bool s_isPaused = true;
static int s_viewMode = 0;
static bool s_dbEditorNeedsOpening = false;

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

  state.requiresRedraw = false;

  // 1. Toolbar (Top)
  DrawToolbar(state, settings, s_isPaused, s_viewMode);

  // 2. Architect Panel (Left)
  DrawArchitectPanel(state, buffers, settings, terrain, graph, s_activeTab,
                     s_paintMode, s_brushSize, s_brushSpeed,
                     state.requiresRedraw);

  // Mouse Logic for Hovered Index & Painting
  static int s_hoveredIndex = -1;
  ImVec2 mousePos = ImGui::GetMousePos();
  if (mousePos.x >= state.viewX && mousePos.x < state.viewX + state.viewW &&
      mousePos.y >= state.viewY && mousePos.y < state.viewY + state.viewH) {

    float relativeY = mousePos.y - state.viewY;
    float normX = (mousePos.x - state.viewX) / (float)state.viewW;
    float normY = 1.0f - (relativeY / (float)state.viewH);

    // Inverse Transform (Screen -> World)
    // Shader: Pos = (World - Offset) * Zoom
    // Inverse: World = (Pos / Zoom) + Offset
    float zoom = pow(4.0f, settings.zoomLevel);

    // normX/Y are 0..1 relative to viewport
    // But they represent "Screen Pos" in the shader's Normalized Device
    // Coordinates context (mapped 0..1)

    float worldX = (normX / zoom) + settings.viewOffset[0];
    float worldY = (normY / zoom) + settings.viewOffset[1];

    int mapX = (int)(worldX * 1000);
    int mapY = (int)(worldY * 1000);

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

  // Fill Output State
  state.isPaused = s_isPaused;
  state.activeTab = s_activeTab;
  state.viewMode = s_viewMode;

  if (s_showDBEditor)
    DrawDatabaseEditor(&s_showDBEditor);

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
  // View Modes - Updated to Combo for space
  const char *viewItems[] = {"Satellite (Biomes)", "Political (Chaos)",
                             "Economic (Wealth)",  "Heightmap (Gray)",
                             "Heightmap (Heat)",   "Topographical"};
  ImGui::Combo("View Mode", &viewMode, viewItems, 6);

  ImGui::SameLine(dim.screenWidth - 250);
  if (ImGui::Button("EDIT RULES / JSON")) {
    s_showDBEditor = !s_showDBEditor;
    if (s_showDBEditor) {
      s_dbEditorNeedsOpening = true;
    }
  }

  ImGui::End();
}

void GuiController::DrawArchitectPanel(
    const GuiState &dim, WorldBuffers &buffers, WorldSettings &settings,
    TerrainController &terrain, NeighborGraph &graph, int &activeTab,
    int &paintMode, float &brushSize, float &brushSpeed, bool &requiresRedraw) {

  // UI State Variables (static for now as requested)
  static bool forceEdges = false;
  static float edgeFadeDist = 50.0f;

  int side = (int)std::sqrt(buffers.count);
  if (side == 0)
    side = 1000;

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
          terrain.EnforceOceanEdges(buffers, side, edgeFadeDist);

        for (int i = 0; i < 100; ++i) {
          HydrologySim::Update(buffers, graph, settings);
          DisasterSystem::Update(buffers, settings);
        }

        requiresRedraw = true; // Full regen always needs redraw
      }

      ImGui::SeparatorText("Global Modifiers");
      if (ImGui::SliderFloat("Sea Level", &settings.seaLevel, -1.0f, 1.0f)) {
        requiresRedraw = true;
      }

      if (ImGui::Checkbox("Ocean Borders", &forceEdges)) {
        if (forceEdges) {
          terrain.EnforceOceanEdges(buffers, side, edgeFadeDist);
          requiresRedraw = true;
        }
      }
      if (forceEdges) {
        ImGui::Indent();
        if (ImGui::SliderFloat("Fade Dist", &edgeFadeDist, 10.0f, 200.0f)) {
          // Re-apply if dragging slider? Might be expensive.
          // Let's apply on Release or just apply always if it's fast enough.
          terrain.EnforceOceanEdges(buffers, side, edgeFadeDist);
          requiresRedraw = true;
        }
        ImGui::Unindent();
      }

      if (ImGui::Button("Smooth Map")) {
        // Three passes for stronger effect
        terrain.SmoothTerrain(buffers, side);
        terrain.SmoothTerrain(buffers, side);
        terrain.SmoothTerrain(buffers, side);
        requiresRedraw = true;
      }
      ImGui::SameLine();
      if (ImGui::Button("Roughen Coast")) {
        terrain.RoughenCoastlines(buffers, side, settings.seaLevel);
        requiresRedraw = true;
      }

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

    // TAB B: Climate & Environment (New)
    if (ImGui::BeginTabItem("Climate")) {
      ImGui::SeparatorText("Zones & Temperature");
      ImGui::SliderFloat("Polar Temp", &settings.tempZonePolar, 0.0f, 1.0f);
      ImGui::SliderFloat("Temperate Temp", &settings.tempZoneTemperate, 0.0f,
                         1.0f);
      ImGui::SliderFloat("Tropical Temp", &settings.tempZoneTropical, 0.0f,
                         1.0f);
      ImGui::SliderFloat("Global Temp Mod", &settings.globalTempModifier, 0.0f,
                         2.0f);

      ImGui::SeparatorText("Wind Zones (N -> S)");
      for (int i = 0; i < 5; ++i) {
        char label[32];
        sprintf(label, "Zone %d Dir", i);
        ImGui::SliderFloat(label, &settings.windZonesDir[i], -3.14f, 3.14f);
        sprintf(label, "Zone %d Str", i);
        ImGui::SliderFloat(label, &settings.windZonesStr[i], 0.0f, 2.0f);
      }
      ImGui::SliderFloat("Global Wind", &settings.globalWindStrength, 0.0f,
                         2.0f);

      ImGui::SeparatorText("Rainfall");
      ImGui::SliderFloat("Global Rainfall", &settings.rainfallModifier, 0.0f,
                         3.0f);

      ImGui::EndTabItem();
    }

    // TAB C: Rivers & Disasters (New)
    if (ImGui::BeginTabItem("Disasters")) {
      ImGui::SeparatorText("Hydrology");
      ImGui::InputInt("River Count", &settings.riverCount);
      ImGui::Checkbox("Flow to Ocean", &settings.riverFlowToOcean);
      ImGui::SliderFloat("Max River Size", &settings.riverMaxSize, 1.0f, 50.0f);
      ImGui::Checkbox("Show Ice", &settings.showIce);

      ImGui::SeparatorText("Disasters");

      auto DrawDisasterControl =
          [&](const char *name, WorldSettings::DisasterSetting &ds, int type) {
            ImGui::PushID(type);
            ImGui::Text("%s", name);
            ImGui::SameLine();
            ImGui::Checkbox("##On", &ds.enabled);
            if (ds.enabled) {
              ImGui::SliderFloat("freq", &ds.frequency, 0.0001f, 0.01f, "%.4f");
              ImGui::SameLine();
              ImGui::SetNextItemWidth(60);
              ImGui::SliderFloat("str", &ds.strength, 0.1f, 2.0f);
              ImGui::SameLine();
              if (ImGui::Button("Spawn")) {
                int idx = rand() % buffers.count;
                DisasterSystem::Trigger(buffers, type, idx, ds.strength);
                requiresRedraw = true;
              }
            }
            ImGui::PopID();
          };

      DrawDisasterControl("Earthquake", settings.quakeSettings, 0);
      DrawDisasterControl("Tsunami", settings.tsunamiSettings, 1);
      DrawDisasterControl("Tornado", settings.tornadoSettings, 2);
      DrawDisasterControl("Hurricane", settings.hurricaneSettings, 3);
      DrawDisasterControl("Wildfire", settings.wildfireSettings, 4);
      DrawDisasterControl("Flood", settings.floodSettings, 5);
      DrawDisasterControl("Drought", settings.droughtSettings, 6);

      ImGui::EndTabItem();
    }

    // TAB D: Life & Civ
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
  if (s_dbEditorNeedsOpening) {
    ImGui::OpenPopup("Database Editor");
    s_dbEditorNeedsOpening = false;
  }

  if (ImGui::BeginPopupModal("Database Editor", p_open,
                             ImGuiWindowFlags_AlwaysAutoResize)) {
    if (ImGui::BeginTabBar("DBTabs")) {
      if (ImGui::BeginTabItem("Resources")) {
        for (auto &r : AssetManager::resourceRegistry) {
          ImGui::Text("%s ($%.2f)", r.name.c_str(), r.value);
        }
        ImGui::EndTabItem();
      }

      if (ImGui::BeginTabItem("Biomes")) {
        static int selectedBiome = -1;

        // List of Biomes (Left Side)
        ImGui::BeginChild("BiomeList", ImVec2(150, 0), true);
        for (int i = 0; i < (int)AssetManager::biomeRegistry.size(); ++i) {
          if (ImGui::Selectable(AssetManager::biomeRegistry[i].name.c_str(),
                                selectedBiome == i)) {
            selectedBiome = i;
          }
        }
        if (ImGui::Button("Add New")) {
          BiomeDef b;
          b.id = (int)AssetManager::biomeRegistry.size();
          b.name = "New Biome";
          b.color[0] = 1.0f;
          b.color[1] = 0.0f;
          b.color[2] = 1.0f;
          b.minTemp = 0;
          b.maxTemp = 1;
          b.minMoisture = 0;
          b.maxMoisture = 1;
          b.minHeight = 0;
          b.maxHeight = 1;
          b.scarcity = 0.0f;
          AssetManager::biomeRegistry.push_back(b);
          selectedBiome = (int)AssetManager::biomeRegistry.size() - 1;
        }
        ImGui::EndChild();

        ImGui::SameLine();

        // Editor (Right Side)
        if (selectedBiome >= 0 &&
            selectedBiome < (int)AssetManager::biomeRegistry.size()) {
          BiomeDef &b = AssetManager::biomeRegistry[selectedBiome];
          ImGui::BeginGroup();
          char nameBuf[64];
          std::strncpy(nameBuf, b.name.c_str(), sizeof(nameBuf));
          if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf))) {
            b.name = std::string(nameBuf);
          }
          ImGui::ColorEdit3("Color", b.color);
          ImGui::SeparatorText("Conditions");
          ImGui::DragFloatRange2("Height", &b.minHeight, &b.maxHeight, 0.01f,
                                 -1.0f, 1.0f);
          ImGui::DragFloatRange2("Temp", &b.minTemp, &b.maxTemp, 0.01f, -1.0f,
                                 2.0f);
          ImGui::DragFloatRange2("Moisture", &b.minMoisture, &b.maxMoisture,
                                 0.01f, 0.0f, 2.0f);
          ImGui::SliderFloat("Scarcity", &b.scarcity, 0.0f, 1.0f);

          if (ImGui::Button("Delete Biome")) {
            AssetManager::biomeRegistry.erase(
                AssetManager::biomeRegistry.begin() + selectedBiome);
            selectedBiome = -1;
          }
          ImGui::EndGroup();
        }
        ImGui::EndTabItem();
      }

      // TAB: AGENTS
      if (ImGui::BeginTabItem("Agents")) {
        static int selectedAgent = -1;
        ImGui::BeginChild("AgentList", ImVec2(150, 0), true);
        for (int i = 0; i < (int)AssetManager::agentRegistry.size(); ++i) {
          if (ImGui::Selectable(AssetManager::agentRegistry[i].name.c_str(),
                                selectedAgent == i)) {
            selectedAgent = i;
          }
        }
        if (ImGui::Button("Add New Agent")) {
          AssetManager::CreateNewAgent();
          selectedAgent = (int)AssetManager::agentRegistry.size() - 1;
        }
        ImGui::EndChild();
        ImGui::SameLine();

        if (selectedAgent >= 0 &&
            selectedAgent < (int)AssetManager::agentRegistry.size()) {
          AgentDefinition &a = AssetManager::agentRegistry[selectedAgent];
          ImGui::BeginChild("AgentSettings", ImVec2(0, 0), false);
          char aName[64];
          std::memset(aName, 0, 64);
          std::strncpy(aName, a.name.c_str(), 63);
          if (ImGui::InputText("Name", aName, 64))
            a.name = aName;

          const char *types[] = {"Flora", "Fauna", "Civilized"};
          int tIdx = (int)a.type;
          if (ImGui::Combo("Type", &tIdx, types, 3))
            a.type = (AgentType)tIdx;
          ImGui::ColorEdit3("Color", a.color);

          ImGui::SeparatorText("Simulation");
          ImGui::SliderFloat("Ideal Temp", &a.idealTemp, 0, 1);
          ImGui::SliderFloat("Ideal Moist", &a.idealMoisture, 0, 1);
          ImGui::SliderFloat("Resilience", &a.resilience, 0, 1);
          ImGui::SliderFloat("Expansion", &a.expansionRate, 0, 1);
          ImGui::SliderFloat("Aggression", &a.aggression, 0, 1);

          ImGui::EndChild();
        }
        ImGui::EndTabItem();
      }
      ImGui::EndTabBar();
    }
    ImGui::Separator();
    if (ImGui::Button("Close", ImVec2(120, 0)))
      *p_open = false;
    ImGui::EndPopup();
  }
}
