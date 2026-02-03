#define STB_IMAGE_IMPLEMENTATION
#include "../../include/stb_image.h" // Ensure this exists in your include folder
#include "../../include/WorldEngine.hpp"
#include <iostream>
#include <algorithm>
#include <cmath>

// Standalone function to load data
void LoadHeightmapData(const char* path, WorldBuffers& buffers, uint32_t count) {
    int width, height, channels;

    // Load image (Force 1 channel = Grayscale)
    unsigned char* data = stbi_load(path, &width, &height, &channels, 1);

    if (!data) {
        std::cerr << "[ERROR] Failed to load heightmap: " << path << std::endl;
        return;
    }

    std::cout << "[IO] Loading heightmap (" << width << "x" << height << ")..." << std::endl;

    // Map image pixels to our 1D Point Cloud
    // Note: Since our world is a point cloud, we sample the image based on the point's Normalized Position (0.0-1.0)
    for (uint32_t i = 0; i < count; ++i) {
        // Convert Unit Coordinates to Pixel Coordinates
        int x = (int)(buffers.posX[i] * (width - 1));
        int y = (int)(buffers.posY[i] * (height - 1));

        // Safety Clamp
        x = std::clamp(x, 0, width - 1);
        y = std::clamp(y, 0, height - 1);

        // Read pixel value (0-255) and normalize to float (0.0-1.0)
        unsigned char pixelVal = data[y * width + x];

        if (buffers.height)
            buffers.height[i] = pixelVal / 255.0f;
    }

    stbi_image_free(data);
    std::cout << "[IO] Terrain import complete." << std::endl;
}
