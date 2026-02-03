#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "../../include/SimulationModules.hpp"
#include <vector>
#include <random>
#include <cmath>
#include <algorithm>

// A spatial grid helps find nearby points quickly
struct Grid {
    int width, height;
    float cellSize;
    int* cells;

    Grid(int w, int h, float c) : width(w), height(h), cellSize(c) {
        cells = new int[width * height];
        for (int i = 0; i < width * height; ++i) cells[i] = -1;
    }
    ~Grid() { delete[] cells; }
};

// Start with a point in the center
void AddPoint(float x, float y, WorldBuffers& buffers, Grid& grid, std::vector<int>& active, uint32_t& count) {
    if (buffers.posX) buffers.posX[count] = x;
    if (buffers.posY) buffers.posY[count] = y;
    int gx = (int)(x / grid.cellSize);
    int gy = (int)(y / grid.cellSize);
    if (gx >= 0 && gx < grid.width && gy >= 0 && gy < grid.height) {
        grid.cells[gx + gy * grid.width] = count;
    }
    active.push_back(count);
    count++;
}

bool IsValid(float x, float y, float w, float h, float rSq, Grid& grid, WorldBuffers& buffers) {
    if (x < 0 || x >= w || y < 0 || y >= h) return false;
    int gx = (int)(x / grid.cellSize);
    int gy = (int)(y / grid.cellSize);

    for (int i = std::max(0, gx - 2); i <= std::min(grid.width - 1, gx + 2); ++i) {
        for (int j = std::max(0, gy - 2); j <= std::min(grid.height - 1, gy + 2); ++j) {
            int neighborIdx = grid.cells[i + j * grid.width];
            if (neighborIdx != -1) {
                float dx = buffers.posX[neighborIdx] - x;
                float dy = buffers.posY[neighborIdx] - y;
                if (dx * dx + dy * dy < rSq) return false;
            }
        }
    }
    return true;
}

void GeneratePoisson(WorldBuffers& buffers, WorldSettings& settings) {
    uint32_t count = settings.cellCount;
    // Estimate map size from count assuming density
    float mapWidth = std::sqrt((float)count); 
    float mapHeight = mapWidth;
    
    // Radius is calculated based on area and desired point count
    float radius = std::sqrt((mapWidth * mapHeight) / (count * 0.8f));
    float radiusSq = radius * radius;
    float cellSize = radius / std::sqrt(2);

    int gridW = (int)std::ceil(mapWidth / cellSize);
    int gridH = (int)std::ceil(mapHeight / cellSize);
    Grid grid(gridW, gridH, cellSize);

    std::vector<int> activeList;
    std::mt19937 gen(1337); // Consistent seed
    std::uniform_real_distribution<float> dist(0, 1);

    uint32_t currentCount = 0;

    AddPoint(mapWidth / 2, mapHeight / 2, buffers, grid, activeList, currentCount);

    while (!activeList.empty() && currentCount < count) {
        int idx = std::uniform_int_distribution<int>(0, activeList.size() - 1)(gen);
        int parentIdx = activeList[idx];
        bool found = false;

        for (int i = 0; i < 30; ++i) { // 30 attempts per point
            float angle = dist(gen) * 2 * M_PI;
            float dist_r = radius + dist(gen) * radius;
            
            float pX = buffers.posX[parentIdx];
            float pY = buffers.posY[parentIdx];
            
            float newX = pX + std::cos(angle) * dist_r;
            float newY = pY + std::sin(angle) * dist_r;

            if (IsValid(newX, newY, mapWidth, mapHeight, radiusSq, grid, buffers)) {
                AddPoint(newX, newY, buffers, grid, activeList, currentCount);
                found = true;
                break;
            }
        }
        if (!found) activeList.erase(activeList.begin() + idx);
    }
}
