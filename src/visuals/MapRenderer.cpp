#include "../../include/MapRenderer.hpp"
#include "../../include/AssetManager.hpp"
#include <GL/glew.h>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

void MapRenderer::Initialize(WorldBuffers &b) {
  // Set up OpenGL Buffers
  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vboPos);
  glGenBuffers(1, &vboColor);

  glBindVertexArray(vao);

  // Position Buffer (X, Y)
  glBindBuffer(GL_ARRAY_BUFFER, vboPos);
  glBufferData(GL_ARRAY_BUFFER, b.count * 2 * sizeof(float), NULL,
               GL_STATIC_DRAW);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void *)0);
  glEnableVertexAttribArray(0);

  // Color Buffer (R, G, B)
  glBindBuffer(GL_ARRAY_BUFFER, vboColor);
  glBufferData(GL_ARRAY_BUFFER, b.count * 3 * sizeof(float), NULL,
               GL_DYNAMIC_DRAW);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
  glEnableVertexAttribArray(1);

  glBindVertexArray(0);
}

void MapRenderer::UpdateVisuals(WorldBuffers &b, const WorldSettings &s,
                                int viewMode) {
  std::vector<float> colors;
  colors.reserve(b.count * 3);

  for (int i = 0; i < b.count; ++i) {
    float r = 0, g = 0, bl = 0;

    if (viewMode == 0) { // PHYSICAL (Height/Water)
      if (b.height[i] < s.seaLevel) {
        r = 0.1f;
        g = 0.2f;
        bl = 0.5f + b.height[i] * 0.2f;
      } else {
        float h = (b.height[i] - s.seaLevel) / (1.0f - s.seaLevel);
        r = 0.2f + h * 0.5f;
        g = 0.5f + h * 0.3f;
        bl = 0.2f;
      }
    } else if (viewMode == 1) { // TEMPERATURE
      r = b.temperature[i];
      g = 0.2f;
      bl = 1.0f - b.temperature[i];
    } else if (viewMode == 2) { // MOISTURE
      r = 0.2f;
      g = b.moisture[i];
      bl = 0.8f;
    } else if (viewMode == 3) { // BIOMES
      // Simple coloring logic
      if (b.height[i] < s.seaLevel) {
        r = 0.05f;
        g = 0.1f;
        bl = 0.4f;
      } else {
        // Tundra
        if (b.temperature[i] < 0.3f) {
          r = 0.8f;
          g = 0.9f;
          bl = 0.9f;
        }
        // Desert
        else if (b.moisture[i] < 0.2f) {
          r = 0.9f;
          g = 0.8f;
          bl = 0.4f;
        }
        // Forest / Jungle
        else if (b.moisture[i] > 0.6f) {
          r = 0.1f;
          g = 0.5f;
          bl = 0.1f;
        }
        // Grassland
        else {
          r = 0.4f;
          g = 0.7f;
          bl = 0.2f;
        }
      }
    } else if (viewMode == 4) { // DIPLOMATIC (Factions)
      if (b.factionID[i] == -1) {
        r = 0.1f;
        g = 0.1f;
        bl = 0.1f;
      } else {
        // Deterministic color from ID
        int id = b.factionID[i];
        r = (float)((id * 123) % 255) / 255.0f;
        g = (float)((id * 456) % 255) / 255.0f;
        bl = (float)((id * 789) % 255) / 255.0f;
      }
    } else if (viewMode == 5) { // ECONOMIC (Resources & Settlements)
      // Base map is dark grayscale
      float h = b.height[i];
      r = g = bl = (h < s.seaLevel) ? 0.1f : 0.2f + (h * 0.2f);

      // Draw Resources over base map
      if (b.resourceType != nullptr && b.resourceAmount != nullptr && b.resourceAmount[i] > 0.0f) {
          uint8_t res = b.resourceType[i];
          if (res == 1) { r = 0.2f; g = 0.6f; bl = 0.9f; } // Fish (Blue)
          else if (res == 2) { r = 0.5f; g = 0.5f; bl = 0.5f; } // Iron (Grey)
          else if (res == 3) { r = 0.9f; g = 0.8f; bl = 0.1f; } // Precious Metals (Gold)
          else if (res == 4) { r = 0.4f; g = 0.8f; bl = 0.2f; } // Timber (Green)
          else if (res == 5) { r = 0.8f; g = 0.4f; bl = 0.2f; } // Crops (Orange)
      }

      // Draw Settlements brightly on top of resources
      if (b.population != nullptr && b.population[i] > 0) {
          if (b.civTier != nullptr && b.civTier[i] > 3) {
             r = 1.0f; g = 0.0f; bl = 0.0f; // Big Cities: Red
          } else {
             r = 1.0f; g = 1.0f; bl = 1.0f; // Small Towns: White
          }
      }
    }

    colors.push_back(r);
    colors.push_back(g);
    colors.push_back(bl);
  }

  // Update GPU Buffer
  glBindBuffer(GL_ARRAY_BUFFER, vboColor);
  glBufferSubData(GL_ARRAY_BUFFER, 0, colors.size() * sizeof(float),
                  colors.data());
}

void MapRenderer::Render(const WorldSettings &s) {
  glBindVertexArray(vao);
  // Shader activation and uniform setting would go here in a real impl
  // glUseProgram(m_shader);
  // glUniform1f(glGetUniformLocation(m_shader, "zoom"), s.zoomLevel);

  glDrawArrays(GL_POINTS, 0, 1000000); // Draw 1M cells as points
  glBindVertexArray(0);
}
