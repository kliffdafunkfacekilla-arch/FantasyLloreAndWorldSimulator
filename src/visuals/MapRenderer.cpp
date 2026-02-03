#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <cmath>
#include <iostream>
#include "../../include/WorldEngine.hpp"

class MapRenderer {
public:
    GLuint vao;
    GLuint vbo_pos;
    GLuint vbo_color;
    std::vector<float> colorBuffer; // CPU-side scratchpad for colors

    // 1. Setup: Allocates GPU memory once
    void Setup(WorldBuffers& b) {
        // Resize CPU color buffer to match cell count (3 floats per cell: R, G, B)
        colorBuffer.resize(b.count * 3);

        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        // --- Position Buffer (Static - never changes after generation) ---
        glGenBuffers(1, &vbo_pos);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_pos);
        // We use dynamic draw in case you re-generate the mesh, but mostly it's static
        glBufferData(GL_ARRAY_BUFFER, b.count * 2 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);

        // Push initial positions (X, Y)
        // Note: Requires Interleaved data or separate arrays.
        // For performance here, we assume b.posX and b.posY are combined or we push them sequentially.
        // *Optimization:* We create a temporary interleaved buffer for the GPU upload.
        std::vector<float> posData(b.count * 2);
        for(uint32_t i=0; i < b.count; ++i) {
            posData[i*2] = b.posX[i]; // X (0.0 - 1.0)
            posData[i*2+1] = b.posY[i]; // Y (0.0 - 1.0)
        }
        glBufferSubData(GL_ARRAY_BUFFER, 0, posData.size() * sizeof(float), posData.data());

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // --- Color Buffer (Dynamic - updates every frame) ---
        glGenBuffers(1, &vbo_color);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_color);
        glBufferData(GL_ARRAY_BUFFER, b.count * 3 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);

        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
    }

    // 2. CPU -> GPU Data Transfer (The "Colorizer")
    void UpdateVisuals(WorldBuffers& b, const WorldSettings& s) {
        for (uint32_t i = 0; i < b.count; ++i) {
            uint32_t idx = i * 3;

            // PRIORITY 1: Factions (Political View)
            if (s.enableFactions && b.factionID[i] > 0) {
                // Simple Hash to give each faction a distinct color
                int fid = b.factionID[i];
                float r = (float)((fid * 37) % 255) / 255.0f;
                float g = (float)((fid * 113) % 255) / 255.0f;
                float b_col = (float)((fid * 200) % 255) / 255.0f;

                // Add a "Red Glow" for conflict zones if chaos is high
                if (b.chaos[i] > 0.5f) { r = 1.0f; g = 0.0f; b_col = 0.0f; }

                colorBuffer[idx] = r;
                colorBuffer[idx+1] = g;
                colorBuffer[idx+2] = b_col;
            }
            // PRIORITY 2: Physical Terrain (Natural View)
            else {
                float h = b.height[i];

                if (h < s.seaLevel) {
                    // OCEAN: Depth darkening
                    float depth = h / s.seaLevel;
                    colorBuffer[idx] = 0.0f;
                    colorBuffer[idx+1] = 0.2f * depth;
                    colorBuffer[idx+2] = 0.4f + (0.4f * depth);
                } else {
                    // LAND: Biome approximation
                    // Check Temperature for Snow/Ice
                    if (b.temperature[i] < 0.15f) {
                        colorBuffer[idx] = 0.9f; colorBuffer[idx+1] = 0.95f; colorBuffer[idx+2] = 1.0f; // White
                    }
                    // Check Moisture for Desert vs Forest
                    else if (b.moisture[i] < 0.1f) {
                        colorBuffer[idx] = 0.8f; colorBuffer[idx+1] = 0.7f; colorBuffer[idx+2] = 0.4f; // Sand
                    }
                    else {
                        // Forest/Grass (Green intensity based on height)
                        colorBuffer[idx] = 0.1f;
                        colorBuffer[idx+1] = 0.4f + (h * 0.3f);
                        colorBuffer[idx+2] = 0.1f;
                    }
                }
            }
        }

        // Upload to GPU
        glBindBuffer(GL_ARRAY_BUFFER, vbo_color);
        glBufferSubData(GL_ARRAY_BUFFER, 0, colorBuffer.size() * sizeof(float), colorBuffer.data());
    }

    // 3. Draw Call
    void Render(const WorldSettings& s) {
        // Uniforms should be set here if shader program is active in main loop
        // We assume glUseProgram is called in main.cpp before this

        glBindVertexArray(vao);
        glDrawArrays(GL_POINTS, 0, colorBuffer.size() / 3);
    }
};
