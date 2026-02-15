#pragma once
#include <cstdlib>
#include <string>

namespace SagaConfig {
// T.A.L.E.W.E.A.V.E.R.S. Global Configuration
// Data Hub Path — reads SAGA_DATA_HUB env var, falls back to default
inline std::string GetDataHub() {
  const char *env = std::getenv("SAGA_DATA_HUB");
  if (env && env[0] != '\0') {
    std::string path(env);
    // Remove surrounding quotes if present
    if (path.size() >= 2 && path.front() == '"' && path.back() == '"') {
      path = path.substr(1, path.size() - 2);
    }
    // Remove trailing quote if lingering
    while (!path.empty() && (path.back() == '"' || path.back() == '\'')) {
      path.pop_back();
    }
    // Normalize separator
    if (!path.empty() && path.back() != '/' && path.back() != '\\')
      path += '/';
    return path;
  }
  // Default fallback (your machine)
  return "C:/Users/krazy/Documents/GitHub/SAGA_Global_Data/";
}

// Shared Data Hub Path
inline const std::string DATA_HUB = GetDataHub();

// Core Data Files
inline const std::string LORE_JSON = DATA_HUB + "lore.json";
inline const std::string RULES_JSON = DATA_HUB + "rules.json";
inline const std::string TEMPLATE_JSON = DATA_HUB + "templates.json";
inline const std::string CALENDAR_JSON = DATA_HUB + "calendar.json";
inline const std::string MANUAL_MD = DATA_HUB + "manual.md";

// Simulation / World Files
inline const std::string WORLD_MAP = DATA_HUB + "world.map";
inline const std::string HISTORY_DIR = DATA_HUB + "history/";
inline const std::string SESSIONS_DIR = DATA_HUB + "sessions/";
} // namespace SagaConfig
