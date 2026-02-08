#pragma once
#include "Lore.hpp"
#include "Simulation.hpp"
#include "Terrain.hpp"
#include "WorldEngine.hpp"
#include "imgui.h"

// Output structure to drive Main Loop
struct GuiState {
  int screenWidth, screenHeight;
  int topBarHeight, bottomPanelHeight;
  int leftPanelWidth, rightPanelWidth;

  // Calculated Viewport
  int viewX, viewY, viewW, viewH;

  // State from UI to Simulation/Main
  bool isPaused;
  int activeTab; // 0=Geo, 1=Life
  int viewMode;  // 0=Sat, 1=Pol, 2=Eco, 3=Magic
};

class GuiController {
public:
  static void Initialize();
  static GuiState DrawMainLayout(WorldBuffers &buffers, WorldSettings &settings,
                                 TerrainController &terrain,
                                 NeighborGraph &graph, int screenW,
                                 int screenH);

private:
  static void DrawToolbar(const GuiState &dim, WorldSettings &settings,
                          bool &isPaused, int &viewMode);
  static void DrawArchitectPanel(const GuiState &dim, WorldBuffers &buffers,
                                 WorldSettings &settings,
                                 TerrainController &terrain,
                                 NeighborGraph &graph, int &activeTab,
                                 int &paintMode, float &brushSize,
                                 float &brushSpeed);
  static void DrawInspector(const GuiState &dim, WorldBuffers &buffers,
                            int hoveredIndex);
  static void DrawChronicle(const GuiState &dim);
  static void DrawDatabaseEditor(bool *p_open);
};
