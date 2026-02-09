#include "../../include/WorldEngine.hpp"
#include "../../include/stb_image.h"
#include <algorithm>
#include <cmath>
#include <iostream>


// Standalone function to load data
void LoadHeightmapData(const char *path, WorldBuffers &buffers,
                       uint32_t count) {
  int width, height, channels;

  // Load image (Force 1 channel = Grayscale)
  unsigned char *data = stbi_load(path, &width, &height, &channels, 1);

  if (!data) {
    std::cerr << "[ERROR] Failed to load heightmap: " << path << std::endl;
    return;
  }

  std::cout << "[IO] Loading heightmap (" << width << "x" << height << ")..."
            << std::endl;

  // Map image pixels to our 1D Point Cloud
  for (uint32_t i = 0; i < count; ++i) {
    // Convert Unit Coordinates to Pixel Coordinates
    int x = (int)(buffers.posX[i] * (width - 1));
    int y = (int)(buffers.posY[i] * (height - 1));

    // Safety Clamp (Manual Implementation as C++17 std::clamp can be finicky in
    // this env)
    if (x < 0)
      x = 0;
    if (x > width - 1)
      x = width - 1;
    if (y < 0)
      y = 0;
    if (y > height - 1)
      y = height - 1;

    // Read pixel value (0-255) and normalize to float (0.0-1.0)
    unsigned char pixelVal = data[y * width + x];

    if (buffers.height)
      buffers.height[i] = pixelVal / 255.0f;
  }

  stbi_image_free(data);
  std::cout << "[IO] Terrain import complete." << std::endl;
}
