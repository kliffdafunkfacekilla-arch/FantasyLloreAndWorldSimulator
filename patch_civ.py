import sys

with open("src/simulation/CivilizationSim.cpp", "r") as f:
    content = f.read()

SEARCH = """  for (uint32_t i = 0; i < b.count; ++i) {
    int id = b.cultureID[i];
    if (id == -1 || id >= (int)AssetManager::agentRegistry.size())
      continue;

    const AgentDefinition &def = AssetManager::agentRegistry[id];
    float pop = (float)b.population[i];

    // Natural Causes
    pop *= DEATH_RATE_OLD_AGE;
    if ((rand() % 10000) / 10000.0f < ACCIDENT_RATE) {
      pop *= 0.8f;
    }

    // Construction
    if (def.type == AgentType::CIVILIZED && b.structureType) {
      float wood = b.GetResource(i, 1);
      float stone = b.GetResource(i, 2); // Assuming Stone is ID 2
      uint8_t &structure = b.structureType[i];

      if (structure == 0 && pop > 100.0f && wood > 50.0f) {
        structure = 1; // Road
        b.AddResource(i, 1, -50.0f);
      } else if (structure == 1 && pop > 500.0f && wood > 100.0f &&
                 stone > 20.0f) {
        structure = 2; // Village
        b.AddResource(i, 1, -100.0f);
        b.AddResource(i, 2, -20.0f);
        LoreScribeNS::LogEvent(0, "FOUNDING", i, "A new village established.");
      } else if (structure == 2 && pop > 2000.0f && stone > 500.0f) {
        structure = 3; // City
        b.AddResource(i, 2, -500.0f);
        LoreScribeNS::LogEvent(0, "CITY", i, "A village grew into a city.");
      }

      // Sync infrastructure level immediately on upgrade
      if (b.infrastructure) {
        if (structure == 1)
          b.infrastructure[i] = std::max(b.infrastructure[i], 0.3f);
        if (structure == 2)
          b.infrastructure[i] = std::max(b.infrastructure[i], 0.6f);
        if (structure == 3)
          b.infrastructure[i] = std::max(b.infrastructure[i], 1.0f);
      }
    }
    b.population[i] = (uint32_t)pop;
  }"""

REPLACE = """  std::vector<int> factionCityPower(AssetManager::agentRegistry.size(), 0);

  for (uint32_t i = 0; i < b.count; ++i) {
    int id = b.cultureID[i];
    if (id != -1 && id < (int)AssetManager::agentRegistry.size()) {
      const AgentDefinition &def = AssetManager::agentRegistry[id];
      if (def.type == AgentType::CIVILIZED && b.structureType) {
        factionCityPower[id] += b.structureType[i];
      }
    }
  }

  std::vector<int> factionTier(AssetManager::agentRegistry.size(), 1);
  for (size_t f = 0; f < factionTier.size(); ++f) {
    int power = factionCityPower[f];
    if (power >= 50) factionTier[f] = 6;
    else if (power >= 30) factionTier[f] = 5;
    else if (power >= 15) factionTier[f] = 4;
    else if (power >= 8) factionTier[f] = 3;
    else if (power >= 3) factionTier[f] = 2;
  }

  for (uint32_t i = 0; i < b.count; ++i) {
    int id = b.cultureID[i];
    if (id == -1 || id >= (int)AssetManager::agentRegistry.size())
      continue;

    const AgentDefinition &def = AssetManager::agentRegistry[id];
    float pop = (float)b.population[i];

    if (b.civTier) {
      b.civTier[i] = factionTier[id];
    }

    // Natural Causes
    pop *= DEATH_RATE_OLD_AGE;
    if ((rand() % 10000) / 10000.0f < ACCIDENT_RATE) {
      pop *= 0.8f;
    }

    // Construction
    if (def.type == AgentType::CIVILIZED && b.structureType) {
      uint8_t &structure = b.structureType[i];

      if (structure == 0 && pop > 100.0f && b.GetResource(i, 1) > 50.0f) {
        structure = 1;
        b.AddResource(i, 1, -50.0f);
        LoreScribeNS::LogEvent(0, "FOUNDING", i, "A new settlement established.");
      } else if (structure == 1 && pop > 250.0f && b.GetResource(i, 0) > 100.0f && b.GetResource(i, 1) > 100.0f && b.GetResource(i, 2) > 20.0f) {
        structure = 2;
        b.AddResource(i, 1, -100.0f);
        b.AddResource(i, 2, -20.0f);
      } else if (structure == 2 && pop > 500.0f && b.GetResource(i, 0) > 200.0f && b.GetResource(i, 1) > 200.0f && b.GetResource(i, 2) > 50.0f) {
        structure = 3;
        b.AddResource(i, 1, -200.0f);
        b.AddResource(i, 2, -50.0f);
        LoreScribeNS::LogEvent(0, "VILLAGE", i, "A settlement grew into a village.");
      } else if (structure == 3 && pop > 1000.0f && b.GetResource(i, 0) > 500.0f && b.wealth[i] > 100.0f && b.GetResource(i, 2) > 200.0f) {
        structure = 4;
        b.AddResource(i, 2, -200.0f);
        b.wealth[i] -= 100.0f;
        LoreScribeNS::LogEvent(0, "TOWN", i, "A village grew into a town.");
      } else if (structure == 4 && pop > 5000.0f && b.GetResource(i, 0) > 1000.0f && b.wealth[i] > 500.0f && b.GetResource(i, 2) > 1000.0f) {
        structure = 5;
        b.AddResource(i, 2, -1000.0f);
        b.wealth[i] -= 500.0f;
        LoreScribeNS::LogEvent(0, "CITY", i, "A town grew into a city.");
      } else if (structure == 5 && pop > 10000.0f && b.GetResource(i, 0) > 5000.0f && b.wealth[i] > 2000.0f && b.GetResource(i, 2) > 5000.0f) {
        structure = 6;
        b.AddResource(i, 2, -5000.0f);
        b.wealth[i] -= 2000.0f;
        LoreScribeNS::LogEvent(0, "METROPOLIS", i, "A city grew into a metropolis.");
      }

      if (b.buildingID && b.buildingID[i] == 0 && b.civTier) {
        if (b.civTier[i] >= 1 && pop > 200.0f && b.GetResource(i, 1) > 50.0f) {
          b.buildingID[i] = 1; // FARM
          b.AddResource(i, 1, -50.0f);
        } else if (b.civTier[i] >= 2 && pop > 300.0f && b.GetResource(i, 1) > 100.0f) {
          b.buildingID[i] = 2; // LUMBER_CAMP
          b.AddResource(i, 1, -100.0f);
        } else if (b.civTier[i] >= 3 && pop > 500.0f && b.GetResource(i, 1) > 200.0f) {
          b.buildingID[i] = 3; // MINE
          b.AddResource(i, 1, -200.0f);
        } else if (b.civTier[i] >= 4 && b.wealth[i] > 200.0f && b.GetResource(i, 2) > 200.0f) {
          b.buildingID[i] = 4; // BARRACKS
          b.AddResource(i, 2, -200.0f);
          b.wealth[i] -= 200.0f;
        } else if (b.civTier[i] >= 5 && b.wealth[i] > 500.0f && b.GetResource(i, 2) > 500.0f) {
          b.buildingID[i] = 5; // WALLS
          b.AddResource(i, 2, -500.0f);
          b.wealth[i] -= 500.0f;
        } else if (b.civTier[i] >= 6 && b.wealth[i] > 1000.0f && b.GetResource(i, 2) > 1000.0f) {
          b.buildingID[i] = 6; // FORTRESS
          b.AddResource(i, 2, -1000.0f);
          b.wealth[i] -= 1000.0f;
        }
      }

      // Sync infrastructure level immediately on upgrade
      if (b.infrastructure) {
        if (structure == 1) b.infrastructure[i] = std::max(b.infrastructure[i], 0.2f);
        if (structure == 2) b.infrastructure[i] = std::max(b.infrastructure[i], 0.4f);
        if (structure == 3) b.infrastructure[i] = std::max(b.infrastructure[i], 0.6f);
        if (structure == 4) b.infrastructure[i] = std::max(b.infrastructure[i], 0.8f);
        if (structure == 5) b.infrastructure[i] = std::max(b.infrastructure[i], 1.0f);
        if (structure == 6) b.infrastructure[i] = std::max(b.infrastructure[i], 1.2f);
      }
    }
    b.population[i] = (uint32_t)pop;
  }"""

if SEARCH in content:
    content = content.replace(SEARCH, REPLACE)
    with open("src/simulation/CivilizationSim.cpp", "w") as f:
        f.write(content)
    print("Patched!")
else:
    print("Search string not found!")
