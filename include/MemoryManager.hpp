#pragma once
#include "WorldEngine.hpp"

class MemoryManager {
public:
  void InitializeWorld(const WorldSettings &settings, WorldBuffers &buffers);
};
