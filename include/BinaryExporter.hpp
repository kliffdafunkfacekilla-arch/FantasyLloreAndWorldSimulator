#pragma once
#include "WorldEngine.hpp"
#include <string>

namespace BinaryExporter {
void SaveWorld(const WorldBuffers &buffers, const std::string &filename);
bool LoadWorld(WorldBuffers &buffers, const std::string &filename);
} // namespace BinaryExporter
