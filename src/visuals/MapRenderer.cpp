#include "../../include/MapRenderer.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>


void MapRenderer::Setup(WorldBuffers &b) {
  // cachedCount = b.count; // We use b.count passed in Render now

  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  // 1. Position X (Location 0)
  glGenBuffers(1, &vbo_posX);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_posX);
  glBufferData(GL_ARRAY_BUFFER, b.count * sizeof(float), nullptr,
               GL_DYNAMIC_DRAW);
  glVertexAttribPointer(0, 1, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(0);

  // 2. Position Y (Location 1)
  glGenBuffers(1, &vbo_posY);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_posY);
  glBufferData(GL_ARRAY_BUFFER, b.count * sizeof(float), nullptr,
               GL_DYNAMIC_DRAW);
  glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(1);

  // 3. Color Buffer (RGB) (Location 2)
  glGenBuffers(1, &vbo_color);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_color);
  glBufferData(GL_ARRAY_BUFFER, b.count * 3 * sizeof(float), nullptr,
               GL_DYNAMIC_DRAW);
  glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(2);

  colorBuffer.resize(b.count * 3);

  // Initial Position Upload
  if (b.posX) {
    glBindBuffer(GL_ARRAY_BUFFER, vbo_posX);
    glBufferSubData(GL_ARRAY_BUFFER, 0, b.count * sizeof(float), b.posX);
  }
  if (b.posY) {
    glBindBuffer(GL_ARRAY_BUFFER, vbo_posY);
    glBufferSubData(GL_ARRAY_BUFFER, 0, b.count * sizeof(float), b.posY);
  }
}

void MapRenderer::Render(WorldBuffers &b) {
  // 1. Update Colors based on Faction/Height
  for (uint32_t i = 0; i < b.count; ++i) {
    uint32_t idx = i * 3;

    // Faction > 0 -> Red
    if (b.factionID && b.factionID[i] > 0) {
      colorBuffer[idx] = 0.9f;
      colorBuffer[idx + 1] = 0.1f;
      colorBuffer[idx + 2] = 0.1f;
    } else {
      // Height (Green) + Moisture (Blue tint)
      float h = b.height[i];
      float m = b.moisture ? b.moisture[i] : 0.0f;

      // Simple logic from user request
      // colorBuffer[i*3] = b.factionID[i] > 0 ? 1.0f : 0.0f;
      // colorBuffer[i*3+1] = b.height[i];
      // colorBuffer[i*3+2] = b.moisture[i];

      // Expanded for better visuals:
      if (h < 0.2f) { // Water (Sea Level implicit or hardcoded for shaderless
                      // logic?)
        // Actually relying on shader usually, but here we bake color
        // Just use the user's logic exactly?
        // "colorBuffer[i*3+1] = b.height[i]; // Green for height"
        colorBuffer[idx] = 0.0f;
        colorBuffer[idx + 1] = h;
        colorBuffer[idx + 2] = m;
      } else {
        colorBuffer[idx] = 0.0f;
        colorBuffer[idx + 1] = h;
        colorBuffer[idx + 2] = m * 0.5f;
      }
    }
  }
  UploadColors(b.count); // Push to GPU

  // 2. Draw
  // Ensure shader is used (User's main snippet puts glUseProgram in main, but
  // better safety here)
  glUseProgram(shaderProgram);

  glEnable(GL_PROGRAM_POINT_SIZE);
  glBindVertexArray(vao);
  glDrawArrays(GL_POINTS, 0, b.count);
}

void MapRenderer::UploadColors(uint32_t count) {
  glBindBuffer(GL_ARRAY_BUFFER, vbo_color);
  glBufferSubData(GL_ARRAY_BUFFER, 0, count * 3 * sizeof(float),
                  colorBuffer.data());
}

// Shader Implementation
GLuint MapRenderer::CompileShader(GLenum type, const std::string &source) {
  GLuint shader = glCreateShader(type);
  const char *src = source.c_str();
  glShaderSource(shader, 1, &src, nullptr);
  glCompileShader(shader);

  // Error Check
  int success;
  char infoLog[512];
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(shader, 512, nullptr, infoLog);
    std::cerr << "[SHADER ERROR] " << infoLog << std::endl;
  }
  return shader;
}

void MapRenderer::LoadShaders(const std::string &vertPath,
                              const std::string &fragPath) {
  std::string vCode, fCode;
  std::ifstream vShaderFile, fShaderFile;

  vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
  fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

  try {
    vShaderFile.open(vertPath);
    fShaderFile.open(fragPath);
    std::stringstream vShaderStream, fShaderStream;
    vShaderStream << vShaderFile.rdbuf();
    fShaderStream << fShaderFile.rdbuf();
    vCode = vShaderStream.str();
    fCode = fShaderStream.str();
  } catch (std::ifstream::failure &e) {
    std::cerr << "[GRAPHICS] Shader file error: " << e.what() << std::endl;
    return;
  }

  GLuint vertex = CompileShader(GL_VERTEX_SHADER, vCode);
  GLuint fragment = CompileShader(GL_FRAGMENT_SHADER, fCode);

  shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, vertex);
  glAttachShader(shaderProgram, fragment);
  glLinkProgram(shaderProgram);

  int success;
  char infoLog[512];
  glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
  if (!success) {
    glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
    std::cerr << "[LINK ERROR] " << infoLog << std::endl;
  }

  glDeleteShader(vertex);
  glDeleteShader(fragment);

  std::cout << "[GRAPHICS] Shader Program Linked ID: " << shaderProgram
            << std::endl;
}
