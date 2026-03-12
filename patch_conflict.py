import sys

with open("src/simulation/ConflictSystem.cpp", "r") as f:
    content = f.read()

SEARCH1 = """      float security = (b.structureType[i] * 10.0f) + (myStr * 0.01f);"""
REPLACE1 = """      float security = (b.structureType[i] * 10.0f) + (myStr * 0.01f);
      if (b.buildingID) {
        if (b.buildingID[i] == 4) security += 20.0f; // BARRACKS
        else if (b.buildingID[i] == 5) security += 50.0f; // WALLS
        else if (b.buildingID[i] == 6) security += 100.0f; // FORTRESS
      }"""

SEARCH2 = """        float damage = myStr * battleDamage * myDef.aggression;
        float defense = 0.0f;
        if (b.structureType)
          defense = (b.structureType[nIdx] * 5.0f);"""
REPLACE2 = """        float damage = myStr * battleDamage * myDef.aggression;
        if (b.civTier) damage *= std::pow(1.5f, (float)b.civTier[i]);
        float defense = 0.0f;
        if (b.structureType) {
          defense = (b.structureType[nIdx] * 5.0f);
        }
        if (b.civTier) {
          defense *= std::pow(1.5f, (float)b.civTier[nIdx]);
        }"""

if SEARCH1 in content and SEARCH2 in content:
    content = content.replace(SEARCH1, REPLACE1)
    content = content.replace(SEARCH2, REPLACE2)
    with open("src/simulation/ConflictSystem.cpp", "w") as f:
        f.write(content)
    print("Patched ConflictSystem.cpp!")
else:
    print("Search strings not found in ConflictSystem.cpp!")
