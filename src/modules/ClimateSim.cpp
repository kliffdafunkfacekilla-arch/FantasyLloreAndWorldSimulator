#include "../../include/SimulationModules.hpp"
#include <algorithm>
#include <cmath>

namespace ClimateSim {

// Helper to get neighbor in wind direction (Approximation)
int GetUpwindNeighbor(int i, int width, int height, float angle) {
  // Convert angle to grid offset (-1, 0, 1)
  int dx = -static_cast<int>(std::round(std::cos(angle)));
  int dy = -static_cast<int>(std::round(std::sin(angle)));

  int x = i % width;
  int y = i / width;

  int nx = std::max(0, std::min(x + dx, width - 1));
  int ny = std::max(0, std::min(y + dy, height - 1));

  return ny * width + nx;
}

void Update(WorldBuffers &b, const WorldSettings &s) {
  if (!b.temperature || !b.moisture)
    return;

  int side = static_cast<int>(std::sqrt(b.count));

  // Simple Advection: Move values from upwind neighbor to current cell
  for (uint32_t i = 0; i < b.count; ++i) {
    float yCell = (float)(i / side) / (float)side;

    float angle = s.windDirEquator;
    float strength = s.windStrengthEquator;
    float tempBias = 0.0f;

    if (yCell < 0.33f) {
      angle = s.windDirNorth;
      strength = s.windStrengthNorth;
      tempBias = s.tempOffsetNorth;
    } else if (yCell > 0.66f) {
      angle = s.windDirSouth;
      strength = s.windStrengthSouth;
      tempBias = s.tempOffsetSouth;
    }

    int upwindIdx = GetUpwindNeighbor(i, side, side, angle);

    if (upwindIdx != (int)i) {
      // Move Temperature
      float tempDiff = b.temperature[upwindIdx] - b.temperature[i];
      b.temperature[i] += tempDiff * strength * 0.1f; // Scaled strength

      // Move Moisture
      float moistDiff = b.moisture[upwindIdx] - b.moisture[i];
      b.moisture[i] += moistDiff * strength * 0.1f;
    }

    // Apply Biases
    b.temperature[i] =
        std::max(0.0f, std::min(b.temperature[i] + (tempBias * 0.001f), 1.0f));

    // Scale global modifiers
    b.temperature[i] =
        std::max(0.0f, std::min(b.temperature[i] * s.globalTempModifier, 1.0f));
    b.moisture[i] =
        std::max(0.0f, std::min(b.moisture[i] * s.rainfallModifier, 1.0f));
  }
}

} // namespace ClimateSim
