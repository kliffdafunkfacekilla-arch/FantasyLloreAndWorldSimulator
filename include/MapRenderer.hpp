#pragma once
#include "WorldEngine.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <vector>

class MapRenderer {
public:
  GLuint vao;
  GLuint vboPos;
  GLuint vboColor;
  std::vector<float> colorBuffer; // CPU-side scratchpad for colors
  bool isDirty = true;

  // 1. Initialization
  void Initialize(WorldBuffers &b);

  // 2. Update Batches
  void UpdateVisuals(WorldBuffers &b, const WorldSettings &s, int viewMode = 0);

  // 3. Draw Call
  void Render(const WorldSettings &s);
};
