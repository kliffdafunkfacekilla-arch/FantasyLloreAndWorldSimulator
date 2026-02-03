#include <vector>
#include <random>
#include "../../include/WorldEngine.hpp"

struct Point { float x, y; };

std::vector<Point> GenerateSeeds(uint32_t count, float width, float height) {
    std::vector<Point> points;
    std::mt19937 gen(42); // Fixed seed for reproducibility
    std::uniform_real_distribution<float> distW(0, width);
    std::uniform_real_distribution<float> distH(0, height);

    // Simple random distribution for now 
    // (We will upgrade to Poisson once the loading screen works)
    for(uint32_t i = 0; i < count; ++i) {
        points.push_back({ distW(gen), distH(gen) });
    }
    
    return points;
}
