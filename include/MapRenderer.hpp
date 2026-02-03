#pragma once
#include "WorldEngine.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <vector>


class MapRenderer {
public:
  GLuint vao;
  GLuint vbo_posX, vbo_posY, vbo_color;

  // Client-side buffer for color construction
  std::vector<float> colorBuffer;

  void Setup(WorldBuffers &b);
  void Render(WorldBuffers &b);
  void UploadColors(uint32_t count);
};
