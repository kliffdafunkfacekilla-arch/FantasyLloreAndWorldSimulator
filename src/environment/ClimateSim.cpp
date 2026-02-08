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
  if (side == 0)
    return;

  // 5 Wind Zones:
  // 0: North Pole (0.0 - 0.1)
  // 1: North Temp (0.1 - 0.3)
  // 2: Equator    (0.3 - 0.7)
  // 3: South Temp (0.7 - 0.9)
  // 4: South Pole (0.9 - 1.0)

  for (uint32_t i = 0; i < b.count; ++i) {
    float yNorm = (float)(i / side) / (float)side; // 0.0 (Top) to 1.0 (Bottom)

    // --- WIND ---
    int zoneIdx = 0;
    if (yNorm < 0.1f)
      zoneIdx = 0;
    else if (yNorm < 0.35f)
      zoneIdx = 1; // North Temp
    else if (yNorm < 0.65f)
      zoneIdx = 2; // Equator
    else if (yNorm < 0.9f)
      zoneIdx = 3; // South Temp
    else
      zoneIdx = 4;

    float angle = s.windZonesDir[zoneIdx];
    float strength = s.windZonesStr[zoneIdx] * s.globalWindStrength;

    // --- TEMPERATURE ---
    // Interpolate between zones
    // 0.0 (N) -> 0.5 (Eq) -> 1.0 (S)
    float targetTemp = 0.0f;
    if (yNorm < 0.5f) {
      // North -> Equator
      float t = yNorm * 2.0f; // 0..1
      targetTemp = s.tempZonePolar * (1.0f - t) + s.tempZoneTropical * t;
    } else {
      // Equator -> South
      float t = (yNorm - 0.5f) * 2.0f; // 0..1
      targetTemp = s.tempZoneTropical * (1.0f - t) + s.tempZonePolar * t;
      // Note: User asked for 3 zones, usually Polar is same both ends.
      // But let's assume Polar applies to both ends if not specified separate.
      // Actually, let's mix in Temperate if t is near 0.5 (Lat 45).
      // Simple linear for robust "3 Zones":
      // Polar (0) -> Temperate (0.25) -> Tropical (0.5) -> Temperate (0.75) ->
      // Polar (1.0)
    }

    // Better Temp Interpolation
    float distFromEq = std::abs(yNorm - 0.5f) * 2.0f; // 0.0(Eq) to 1.0(Pole)
    if (distFromEq < 0.33f) {
      // Tropical Band
      targetTemp = s.tempZoneTropical;
    } else if (distFromEq < 0.66f) {
      // Temperate Band
      targetTemp = s.tempZoneTemperate;
    } else {
      // Polar Band
      targetTemp = s.tempZonePolar;
    }

    // Altitude Cooling
    float h = b.height[i];
    if (h > s.seaLevel) {
      float altFactor = (h - s.seaLevel) / (1.0f - s.seaLevel);
      targetTemp -= altFactor * 0.5f; // Mountains overlap zones
    }

    // Move Air (Advection) - Simplified
    int upwindIdx = GetUpwindNeighbor(i, side, side, angle);
    if (upwindIdx != (int)i) {
      // Pull temp/moisture from upwind
      float tempDiff = b.temperature[upwindIdx] - b.temperature[i];
      b.temperature[i] += tempDiff * strength * 0.2f;

      float moistDiff = b.moisture[upwindIdx] - b.moisture[i];
      b.moisture[i] += moistDiff * strength * 0.2f;
    }

    // Nudging towards Target Temp (Climate Stability)
    b.temperature[i] = b.temperature[i] * 0.95f + targetTemp * 0.05f;

    // Global Modifiers
    b.temperature[i] =
        std::max(0.0f, std::min(b.temperature[i] * s.globalTempModifier, 1.0f));
    b.moisture[i] =
        std::max(0.0f, std::min(b.moisture[i] * s.rainfallModifier, 1.0f));
  }
}

} // namespace ClimateSim
