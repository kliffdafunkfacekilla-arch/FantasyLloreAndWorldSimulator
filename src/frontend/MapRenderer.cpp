#include "MapRenderer.hpp"
#include "../../include/AssetManager.hpp"
#include <algorithm> // for std::min, std::max
#include <cmath>
#include <iostream>
#include <vector>


// 1. Initialization
void MapRenderer::Setup(const WorldBuffers &buffers) {
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
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);

  // Color Attribute (Index 1)
  glBindBuffer(GL_ARRAY_BUFFER, vbo_color);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(1);

  glBindVertexArray(0);
  std::cout << "[LOG] MapRenderer::Setup finished. VAO unbound.\n";
}

// 2. Update Batches
// viewMode: 0=Terrain, 1=Chaos, 2=Economy
void MapRenderer::UpdateVisuals(const WorldBuffers &buffers,
                                const WorldSettings &s, int viewMode) {
  // viewMode:
  // 0=Terrain(Standard/Biome), 1=Chaos, 2=Economy
  // 3=Heightmap(Gray), 4=Heightmap(Color), 5=Topographical

  static int lastViewMode = -1;
  static float lastSeaLevel = -1.0f;

  // Check if anything significant changed
  if (!isDirty && viewMode == lastViewMode && s.seaLevel == lastSeaLevel)
    return;

  lastViewMode = viewMode;
  lastSeaLevel = s.seaLevel;

  colorBuffer.resize(buffers.count * 3);

  for (size_t i = 0; i < buffers.count; ++i) {
    float h = buffers.height[i];
    float r = 0, g = 0, b = 0;

    // --- VIEW MODES ---
    if (viewMode == 3) {
      // Heightmap (Grayscale)
      float val = (h + 1.0f) * 0.5f; // Map -1..1 to 0..1
      r = g = b = std::max(0.0f, std::min(1.0f, val));
    } else if (viewMode == 4) {
      // Heightmap (Heatmap: Blue->Green->Red)
      float val = (h + 1.0f) * 0.5f;
      if (val < 0.5f) {
        // Blue to Green
        r = 0.0f;
        g = val * 2.0f;
        b = 1.0f - (val * 2.0f);
      } else {
        // Green to Red
        r = (val - 0.5f) * 2.0f;
        g = 1.0f - ((val - 0.5f) * 2.0f);
        b = 0.0f;
      }
    } else if (viewMode == 5) {
      // Topographical (Banded)
      float val = (h + 1.0f) * 0.5f;
      float band = floor(val * 20.0f) / 20.0f; // 20 bands
      // Color based on elevation type
      if (val < 0.5f) { // Water
        r = 0.0f;
        g = 0.2f + band * 0.3f;
        b = 0.5f + band * 0.5f;
      } else { // Land
        r = 0.2f + band * 0.8f;
        g = 0.5f + band * 0.3f;
        b = 0.2f;
      }
    } else if (viewMode == 1 && buffers.chaos) {
      // CHAOS VIEW (Purple magic)
      float c = buffers.chaos[i];
      r = c;
      g = 0.0f;
      b = c;
      if (c > 0.01f) {
        r += 0.2f;
        b += 0.4f;
      }
      r = std::min(r, 1.0f);
      b = std::min(b, 1.0f);
    } else if (viewMode == 2 && buffers.wealth) {
      // ECONOMY VIEW
      float w = buffers.wealth ? buffers.wealth[i] : 0.0f;
      float infra = buffers.infrastructure ? buffers.infrastructure[i] : 0.0f;
      r = std::min(1.0f, (w / 100.0f) + infra);
      g = std::min(1.0f, (w / 100.0f));
      b = 0.0f;
    } else {
      // STANDARD TERRAIN (Biomes)
      if (h < s.seaLevel) {
        // Ocean
        float depth = h / (s.seaLevel + 1.0f); // approx depth
        // Let's keep it simple: Deep Ocean vs Ocean vs Coast
        // We can try to use Biomes for ocean too if we want, but usually ocean
        // is handled by seaLevel check foremost. But user wants custom biomes.
        // Let's check Ocean biomes in the loop? Actually, easiest to keep
        // hardcoded water base, OR look up biome.

        // Let's try dynamic lookup for everything!
        // But optimization: only check if we don't return early.
      }

      // DYNAMIC BIOME LOOKUP
      // Iterate through registry. First match wins (or we can add scoring).
      bool matched = false;
      float t = buffers.temperature[i];
      float m = buffers.moisture[i];

      // Default color if no biome matches (Error Pink)
      r = 1.0f;
      g = 0.0f;
      b = 1.0f;

      for (const auto &biome : AssetManager::biomeRegistry) {
        // Check Height
        // If biome.minHeight == -1.0, it ignores lower bound? No, usually -1.0
        // is lowest. Special case: Ocean vs Land. If the biome is meant to be
        // underwater, its maxHeight should be < seaLevel. If the biome is land,
        // its minHeight should be > seaLevel.

        // Actually, let's just stick to raw values.
        // If h is between minH and maxH
        if (h >= biome.minHeight && h <= biome.maxHeight) {
          if (t >= biome.minTemp && t <= biome.maxTemp) {
            if (m >= biome.minMoisture && m <= biome.maxMoisture) {
              // MATCH!
              r = biome.color[0];
              g = biome.color[1];
              b = biome.color[2];

              // Visual noise/texture based on height variation
              float noise = (rand() % 10) / 100.0f;
              r += noise;
              g += noise;
              b += noise;

              matched = true;
              break;
            }
          }
        }
      }

      if (!matched) {
        // Fallback if no biome found (e.g. gaps in definition)
        if (h < s.seaLevel) {
          r = 0;
          g = 0.2f;
          b = 0.6f;
        } // Generic Water
        else {
          r = 0.5f;
          g = 0.5f;
          b = 0.5f;
        } // Generic Land
      }
    }

    colorBuffer[i * 3 + 0] = r;
    colorBuffer[i * 3 + 1] = g;
    colorBuffer[i * 3 + 2] = b;
  }

  // Update Buffers... (Existing code below)
  glBindVertexArray(vao);

  // Update Position Buffer
  glBindBuffer(GL_ARRAY_BUFFER, vbo_pos);
  std::vector<float> posData(buffers.count * 2);
  for (size_t i = 0; i < buffers.count; ++i) {
    posData[i * 2 + 0] = buffers.posX[i];
    posData[i * 2 + 1] = buffers.posY[i];
  }
  glBufferData(GL_ARRAY_BUFFER, posData.size() * sizeof(float), posData.data(),
               GL_DYNAMIC_DRAW);

  // Update Color Buffer
  glBindBuffer(GL_ARRAY_BUFFER, vbo_color);
  glBufferData(GL_ARRAY_BUFFER, colorBuffer.size() * sizeof(float),
               colorBuffer.data(), GL_DYNAMIC_DRAW);

  isDirty = false;
}

// 3. Draw Call
void MapRenderer::Render(const WorldSettings &s) {
  glBindVertexArray(vao);
  size_t pointCount = colorBuffer.size() / 3;

  static int frameCount = 0;
  if (frameCount++ % 300 == 0) {
    std::cout << "[RENDERER] Drawing " << pointCount << " points.\n";
  }

  glDrawArrays(GL_POINTS, 0, pointCount);
}
