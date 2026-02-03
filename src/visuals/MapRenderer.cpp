#include "../../include/MapRenderer.hpp"

void MapRenderer::Setup(WorldBuffers &b) {
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  // 1. Position X (Location 0)
  glGenBuffers(1, &vbo_posX);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_posX);
  glBufferData(GL_ARRAY_BUFFER, b.count * sizeof(float), nullptr,
               GL_DYNAMIC_DRAW);
  glVertexAttribPointer(0, 1, GL_FLOAT, GL_FALSE, 0, 0); // 1 float per vertex
  glEnableVertexAttribArray(0);

  // 2. Position Y (Location 1)
  glGenBuffers(1, &vbo_posY);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_posY);
  glBufferData(GL_ARRAY_BUFFER, b.count * sizeof(float), nullptr,
               GL_DYNAMIC_DRAW);
  glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 0, 0); // 1 float per vertex
  glEnableVertexAttribArray(1);

  // 3. Color Buffer (RGB) (Location 2)
  // This represents Height/Biome/Faction
  glGenBuffers(1, &vbo_color);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_color);
  glBufferData(GL_ARRAY_BUFFER, b.count * 3 * sizeof(float), nullptr,
               GL_DYNAMIC_DRAW);
  glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0,
                        0); // 3 floats per vertex (R,G,B)
  glEnableVertexAttribArray(2);

  // Prepare client buffer
  colorBuffer.resize(b.count * 3);
}

void MapRenderer::Render(WorldBuffers &b) {
  // Update Positions
  if (b.posX) {
    glBindBuffer(GL_ARRAY_BUFFER, vbo_posX);
    glBufferSubData(GL_ARRAY_BUFFER, 0, b.count * sizeof(float), b.posX);
  }
  if (b.posY) {
    glBindBuffer(GL_ARRAY_BUFFER, vbo_posY);
    glBufferSubData(GL_ARRAY_BUFFER, 0, b.count * sizeof(float), b.posY);
  }

  // Draw as Points
  glBindVertexArray(vao);
  glDrawArrays(GL_POINTS, 0, b.count);
}

// Helper to upload color derived from game state
void MapRenderer::UploadColors(uint32_t count) {
  glBindBuffer(GL_ARRAY_BUFFER, vbo_color);
  glBufferSubData(GL_ARRAY_BUFFER, 0, count * 3 * sizeof(float),
                  colorBuffer.data());
}
