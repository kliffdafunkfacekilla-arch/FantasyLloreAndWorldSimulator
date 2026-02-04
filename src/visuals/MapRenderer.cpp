#include "../../include/WorldEngine.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cmath>
#include <iostream>
#include <vector>

class MapRenderer {
public:
  GLuint vao;
  GLuint vbo_pos;
  GLuint vbo_color;
  std::vector<float> colorBuffer; // CPU-side scratchpad for colors

  // 1. Initialization
  void Setup(const WorldBuffers &buffers) {
    std::cout << "[LOG] MapRenderer::Setup called.\n";
    // Create VAO and VBOs
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo_pos);
    glGenBuffers(1, &vbo_color);

    std::cout << "[LOG] Generated VAO: " << vao << ", VBO_Pos: " << vbo_pos
              << ", VBO_Color: " << vbo_color << "\n";

    glBindVertexArray(vao);

    // Position Attribute (Index 0)
    glBindBuffer(GL_ARRAY_BUFFER, vbo_pos);
    // Initial buffer data (null for now, just allocating or verifying)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float),
                          (void *)0);
    glEnableVertexAttribArray(0);

    // Color Attribute (Index 1)
    glBindBuffer(GL_ARRAY_BUFFER, vbo_color);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float),
                          (void *)0);
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    std::cout << "[LOG] MapRenderer::Setup finished. VAO unbound.\n";
  }

  // 2. Update Batches
  void UpdateVisuals(const WorldBuffers &buffers, const WorldSettings &s) {
    colorBuffer.resize(buffers.count * 3);

    for (size_t i = 0; i < buffers.count; ++i) {
      float h = buffers.height[i];
      float r = 0, g = 0, b = 0;

      if (h < s.seaLevel) {
        // Ocean
        float depth = h / s.seaLevel;
        r = 0.0f;
        g = 0.2f * depth;
        b = 0.4f + (0.4f * depth);
      } else {
        // Land
        // Whittaker-like Biome Logic
        float t = buffers.temperature[i];
        float m = buffers.moisture[i];

        // Default (Rock/High Altitude)
        r = 0.5f;
        g = 0.5f;
        b = 0.5f;

        if (t < 0.2f) {
          // Snow/Ice (White)
          r = 0.95f;
          g = 0.95f;
          b = 1.0f;
        } else if (buffers.population && buffers.population[i] > 500) {
          // City (Red/Faction Color)
          r = 1.0f;
          g = 0.0f;
          b = 0.0f;
        } else if (buffers.flux && buffers.flux[i] > 1.0f) {
          // River (Blue) - Color based on flow volume
          float intensity = std::min(buffers.flux[i] * 0.1f, 1.0f);
          r = 0.0f;
          g = 0.2f + intensity * 0.5f;
          b = 0.8f + intensity * 0.2f;
        } else if (m < 0.2f) {
          // Desert (Yellow/Sand)
          r = 0.9f;
          g = 0.8f;
          b = 0.5f;
        } else if (m > 0.6f && t > 0.6f) {
          // Rainforest (Dark Green)
          r = 0.0f;
          g = 0.35f;
          b = 0.1f;
        } else {
          // Plains/Forest (Green)
          // Variation based on height for visual depth
          r = 0.2f;
          g = 0.6f - (h * 0.2f);
          b = 0.1f;
        }
      }

      colorBuffer[i * 3 + 0] = r;
      colorBuffer[i * 3 + 1] = g;
      colorBuffer[i * 3 + 2] = b;
    }

    glBindVertexArray(vao);

    // Update Position Buffer
    glBindBuffer(GL_ARRAY_BUFFER, vbo_pos);
    std::vector<float> posData(buffers.count * 2);
    for (size_t i = 0; i < buffers.count; ++i) {
      posData[i * 2 + 0] = buffers.posX[i];
      posData[i * 2 + 1] = buffers.posY[i];
    }
    glBufferData(GL_ARRAY_BUFFER, posData.size() * sizeof(float),
                 posData.data(), GL_DYNAMIC_DRAW);

    // Update Color Buffer
    glBindBuffer(GL_ARRAY_BUFFER, vbo_color);
    glBufferData(GL_ARRAY_BUFFER, colorBuffer.size() * sizeof(float),
                 colorBuffer.data(), GL_DYNAMIC_DRAW);
  }

  // 3. Draw Call
  void Render(const WorldSettings &s) {
    glBindVertexArray(vao);
    size_t pointCount = colorBuffer.size() / 3;

    static int frameCount = 0;
    if (frameCount++ % 300 == 0) {
      std::cout << "[RENDERER] Drawing " << pointCount << " points.\n";
    }

    glDrawArrays(GL_POINTS, 0, pointCount);
  }
};
