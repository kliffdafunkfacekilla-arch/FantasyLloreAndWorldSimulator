#pragma once
#include "WorldEngine.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <string>
#include <vector>

class MapRenderer {
public:
  GLuint vao;
  GLuint vbo_posX, vbo_posY, vbo_color;
  GLuint shaderProgram;

  // Client-side buffer for color construction
  std::vector<float> colorBuffer;

  // Updated Interface
  void Setup(WorldBuffers &b);
  void Render(WorldBuffers &b);
  void UploadColors(uint32_t count);

  // Internal helper
  // Shader Management
  void LoadShaders(const std::string &vertPath, const std::string &fragPath);

private:
  uint32_t cachedCount = 0;
  GLuint CompileShader(GLenum type, const std::string &source);
};
