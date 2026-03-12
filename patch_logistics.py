import sys

with open("src/simulation/LogisticsSystem.cpp", "r") as f:
    content = f.read()

SEARCH = """      // Grasslands/Forests produce Food (ID 0)
      if (biome == 7 || biome == 8 || biome == 5 || biome == 6) {
        b.AddResource(i, 0, 1.0f * workForce);
      }
      // Forests/Rainforests produce Wood (ID 1)
      if (biome == 8 || biome == 9 || biome == 6 || biome == 10) {
        b.AddResource(i, 1, 0.5f * workForce);
      }
      // Mountains/Hills produce Iron/Stone (ID 2)
      if (biome == 13 || h > 0.7f) {
        b.AddResource(i, 2, 0.3f * workForce);
      }"""

REPLACE = """      // Multipliers based on buildings
      float foodMult = (b.buildingID && b.buildingID[i] == 1) ? 2.0f : 1.0f; // FARM
      float woodMult = (b.buildingID && b.buildingID[i] == 2) ? 2.0f : 1.0f; // LUMBER_CAMP
      float stoneMult = (b.buildingID && b.buildingID[i] == 3) ? 2.0f : 1.0f; // MINE

      // Grasslands/Forests produce Food (ID 0)
      if (biome == 7 || biome == 8 || biome == 5 || biome == 6) {
        b.AddResource(i, 0, 1.0f * workForce * foodMult);
      }
      // Forests/Rainforests produce Wood (ID 1)
      if (biome == 8 || biome == 9 || biome == 6 || biome == 10) {
        b.AddResource(i, 1, 0.5f * workForce * woodMult);
      }
      // Mountains/Hills produce Iron/Stone (ID 2)
      if (biome == 13 || h > 0.7f) {
        b.AddResource(i, 2, 0.3f * workForce * stoneMult);
      }"""

if SEARCH in content:
    content = content.replace(SEARCH, REPLACE)
    with open("src/simulation/LogisticsSystem.cpp", "w") as f:
        f.write(content)
    print("Patched LogisticsSystem.cpp!")
else:
    print("Search string not found in LogisticsSystem.cpp!")
