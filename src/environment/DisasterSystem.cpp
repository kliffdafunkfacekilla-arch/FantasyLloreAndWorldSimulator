#include "Environment.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>


namespace DisasterSystem {

void Trigger(WorldBuffers &b, int type, int index, float strength) {
  if (index < 0 || index >= (int)b.count)
    return;

  int side = (int)std::sqrt(b.count);
  int x = index % side;
  int y = index / side;

  // 0: Earthquake, 1: Tsunami, 2: Tornado, 3: Hurricane, 4: Wildfire, 5: Flood,
  // 6: Drought

  int r = (int)(strength * 20.0f); // Radius
  if (r < 1)
    r = 1;

  for (int cy = y - r; cy <= y + r; ++cy) {
    for (int cx = x - r; cx <= x + r; ++cx) {
      if (cx < 0 || cx >= side || cy < 0 || cy >= side)
        continue;

      int i = cy * side + cx;
      float dx = (float)(cx - x);
      float dy = (float)(cy - y);
      float dist = std::sqrt(dx * dx + dy * dy);
      if (dist > r)
        continue;
      float falloff = 1.0f - (dist / r);

      switch (type) {
      case 0: // Earthquake (Noise/Displacement)
      {
        float noise = ((rand() % 100) / 100.0f - 0.5f) * strength * falloff;
        b.height[i] += noise;
        // Damage buildings
        if (b.infrastructure)
          b.infrastructure[i] =
              std::max(0.0f, b.infrastructure[i] - strength * falloff);
      } break;
      case 1: // Tsunami (Massive Water)
      {
        if (b.flux)
          b.flux[i] += strength * 100.0f * falloff;
      } break;
      case 2: // Tornado (Wind + Structure Damage)
      {
        if (b.infrastructure)
          b.infrastructure[i] =
              std::max(0.0f, b.infrastructure[i] - strength * 5.0f * falloff);
        // Scatter resources?
      } break;
      case 3: // Hurricane (Wind + Rain)
      {
        if (b.moisture)
          b.moisture[i] = std::min(1.0f, b.moisture[i] + strength * falloff);
        if (b.flux)
          b.flux[i] += strength * 50.0f * falloff;
      } break;
      case 4: // Wildfire (Heat + Dryness)
      {
        if (b.temperature)
          b.temperature[i] =
              std::min(1.0f, b.temperature[i] + strength * falloff);
        if (b.moisture)
          b.moisture[i] = std::max(0.0f, b.moisture[i] - strength * falloff);
        // Kill plants? (Need biology module)
      } break;
      case 5: // Flood (Rain)
      {
        if (b.flux)
          b.flux[i] += strength * 50.0f * falloff;
      } break;
      case 6: // Drought (Dryness)
      {
        if (b.moisture)
          b.moisture[i] = std::max(0.0f, b.moisture[i] - strength * falloff);
        if (b.flux)
          b.flux[i] *= (1.0f - strength * falloff);
      } break;
      }
    }
  }
}

void Update(WorldBuffers &b, const WorldSettings &s) {
  // Check random spawn based on frequency
  // We can't check every cell every tick for efficiency.
  // Instead check "Global Chance" once per tick per type.

  auto CheckAndSpawn = [&](const WorldSettings::DisasterSetting &ds, int type) {
    if (ds.enabled && (rand() % 100000) < (ds.frequency * 100000)) {
      // Spawn!
      int idx = rand() % b.count;
      Trigger(b, type, idx, ds.strength);
      std::cout << "[DISASTER] " << type << " spawned at " << idx << "\n";
    }
  };

  CheckAndSpawn(s.quakeSettings, 0);
  CheckAndSpawn(s.tsunamiSettings, 1);
  CheckAndSpawn(s.tornadoSettings, 2);
  CheckAndSpawn(s.hurricaneSettings, 3);
  CheckAndSpawn(s.wildfireSettings, 4);
  CheckAndSpawn(s.floodSettings, 5);
  CheckAndSpawn(s.droughtSettings, 6);
}

} // namespace DisasterSystem
