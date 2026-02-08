#pragma once
#include "../../include/WorldEngine.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <vector>

class MapRenderer {
public:
  GLuint vao;
  GLuint vbo_pos;
  GLuint vbo_color;
  std::vector<float> colorBuffer; // CPU-side scratchpad for colors
  bool isDirty = true;

  // 1. Initialization
  void Setup(const WorldBuffers &buffers);

  // 2. Update Batches
  // viewMode: 0=Terrain, 1=Chaos, 2=Economy
  void UpdateVisuals(const WorldBuffers &buffers, const WorldSettings &s,
                     int viewMode = 0);

  // 3. Draw Call
  void Render(const WorldSettings &s);
};
