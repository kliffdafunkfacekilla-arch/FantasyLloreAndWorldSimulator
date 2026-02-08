
// --- NEW TERRAIN TOOLS ---

void TerrainController::EnforceOceanEdges(WorldBuffers &b, int width,
                                          float fadeDist) {
  if (!b.height)
    return;
  // Simple distance from center logic or edge logic
  // "fadeDist" is norm distance
  // But user passes "1000" as width.

  for (int y = 0; y < width; ++y) {
    for (int x = 0; x < width; ++x) {
      // Distance from nearest edge
      float dx = (float)std::min(x, width - 1 - x);
      float dy = (float)std::min(y, width - 1 - y);
      float dist = std::min(dx, dy);

      if (dist < fadeDist) {
        float factor = dist / fadeDist;
        // Fade towards deep ocean (-1.0) or sea level?
        // Let's fade towards -0.5f
        int idx = y * width + x;
        // b.height[idx] = b.height[idx] * factor - 0.5f * (1.0f - factor);
        b.height[idx] = b.height[idx] * factor + (-1.0f) * (1.0f - factor);
      }
    }
  }
}

void TerrainController::RoughenCoastlines(WorldBuffers &b, int width,
                                          float seaLevel) {
  if (!b.height)
    return;
  for (size_t i = 0; i < b.count; ++i) {
    if (std::abs(b.height[i] - seaLevel) < 0.05f) { // Narrow band
      float noise = ((rand() % 100) / 100.0f - 0.5f) * 0.1f;
      b.height[i] += noise;
    }
  }
}

void TerrainController::ApplyBrush(WorldBuffers &b, int width, int cx, int cy,
                                   float radius, float strength, int mode) {
  if (!b.height)
    return;
  int r = (int)radius;
  int r2 = r * r;

  // Target height for Flatten mode (center of brush)
  float targetHeight = 0.0f;
  if (mode == 2) {
    int centerIdx = cy * width + cx;
    if (centerIdx >= 0 && centerIdx < (int)b.count)
      targetHeight = b.height[centerIdx];
  }

  for (int y = cy - r; y <= cy + r; ++y) {
    for (int x = cx - r; x <= cx + r; ++x) {
      // Bounds check
      if (x < 0 || x >= width || y < 0 || y >= width)
        continue;

      // Circle check
      int dx = x - cx;
      int dy = y - cy;
      if (dx * dx + dy * dy > r2)
        continue;

      // Falloff (Stronger in center)
      float dist = std::sqrt((float)(dx * dx + dy * dy));
      float falloff = 1.0f - (dist / radius);
      if (falloff < 0)
        falloff = 0;

      int i = y * width + x;
      float change = strength * falloff * 0.05f; // Scale down for control

      switch (mode) {
      case 0: // RAISE
        b.height[i] += change;
        break;
      case 1: // LOWER
        b.height[i] -= change;
        break;
      case 2: // FLATTEN
        b.height[i] = b.height[i] +
                      (targetHeight - b.height[i]) * 0.1f * strength * falloff;
        break;
      case 3: // NOISE/ROUGHEN
        b.height[i] += ((rand() % 100) / 1000.0f - 0.05f) * strength * falloff;
        break;
      }
      // Clamp
      if (b.height[i] > 1.0f)
        b.height[i] = 1.0f;
      if (b.height[i] < -1.0f)
        b.height[i] = -1.0f;
    }
  }
}
