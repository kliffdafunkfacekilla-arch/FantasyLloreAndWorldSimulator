#include "../../deps/imgui/imgui.h"
#include "../../include/AssetManager.hpp"
#include <string>
#include <vector>

// Helper to wrap the ugly ImGui map editing logic
void DrawRequirementMap(const char *label, std::map<int, float> &dataMap) {
  ImGui::PushID(label); // <--- FIX: Scope IDs to this specific map
  ImGui::Text("%s", label);

  // 1. LIST EXISTING
  std::vector<int> toRemove;
  for (auto &[resID, amount] : dataMap) {
    ImGui::PushID(resID);

    // Find name for display
    std::string name = "Unknown ID:" + std::to_string(resID);
    for (auto &r : AssetManager::resourceRegistry) {
      if (r.id == resID) {
        name = r.name;
        break;
      }
    }

    if (ImGui::Button("X"))
      toRemove.push_back(resID);
    ImGui::SameLine();
    ImGui::Text("%s", name.c_str());
    ImGui::SameLine();
    ImGui::SetNextItemWidth(80);
    ImGui::DragFloat("##amt", &amount, 0.1f, -1.0f,
                     1.0f); // -1=avoid, 0=neutral, +1=seek

    ImGui::PopID();
  }

  // Remove deleted items
  for (int id : toRemove)
    dataMap.erase(id);

  // 2. ADD NEW
  static int selectedResToAdd = 0;
  if (!AssetManager::resourceRegistry.empty()) {
    // Safe Dropdown
    if (selectedResToAdd >= (int)AssetManager::resourceRegistry.size())
      selectedResToAdd = 0;

    const char *preview =
        AssetManager::resourceRegistry[selectedResToAdd].name.c_str();

    ImGui::SetNextItemWidth(120);
    if (ImGui::BeginCombo("##addSel", preview)) {
      for (int i = 0; i < (int)AssetManager::resourceRegistry.size(); i++) {
        if (ImGui::Selectable(AssetManager::resourceRegistry[i].name.c_str(),
                              selectedResToAdd == i))
          selectedResToAdd = i;
      }
      ImGui::EndCombo();
    }
    ImGui::SameLine();
    if (ImGui::Button("Add")) {
      int realID = AssetManager::resourceRegistry[selectedResToAdd].id;
      dataMap[realID] = 0.0f; // Default neutral
    }
  }
  ImGui::PopID(); // <--- FIX: End scope
}

void DrawDatabaseEditor(bool *p_open) {
  ImGui::SetNextWindowSize(ImVec2(650, 550), ImGuiCond_FirstUseEver);
  if (!ImGui::Begin("Omnis World Database", p_open)) {
    ImGui::End();
    return;
  }

  if (ImGui::BeginTabBar("DBTabs")) {

    // --- TAB 1: RESOURCES ---
    if (ImGui::BeginTabItem("Resources")) {

      // LIST
      ImGui::BeginChild("ResList", ImVec2(150, 0), ImGuiChildFlags_Borders);
      if (ImGui::Button("+ Add Resource")) {
        ResourceDef r;
        r.id = (int)AssetManager::resourceRegistry.size();
        r.name = "New Resource";
        r.value = 1.0f;
        r.isRenewable = true;
        r.regenerationRate = 0.1f;
        r.scarcity = 0.5f;
        r.spawnsInForest = false;
        r.spawnsInMountain = false;
        r.spawnsInDesert = false;
        r.spawnsInOcean = false;
        AssetManager::resourceRegistry.push_back(r);
      }
      static int selectedRes = 0;
      for (int i = 0; i < (int)AssetManager::resourceRegistry.size(); i++) {
        if (ImGui::Selectable(AssetManager::resourceRegistry[i].name.c_str(),
                              selectedRes == i))
          selectedRes = i;
      }
      ImGui::EndChild();

      // DETAILS
      ImGui::SameLine();
      ImGui::BeginGroup();
      if (selectedRes < (int)AssetManager::resourceRegistry.size()) {
        ResourceDef &r = AssetManager::resourceRegistry[selectedRes];
        ImGui::Text("Editing ID: %d", r.id);

        // Name buffer for editing
        static char nameBuf[64];
        strncpy(nameBuf, r.name.c_str(), 63);
        if (ImGui::InputText("Name", nameBuf, 64)) {
          r.name = nameBuf;
        }

        ImGui::InputFloat("Value", &r.value);
        ImGui::Checkbox("Renewable?", &r.isRenewable);
        ImGui::SliderFloat("Regen Rate", &r.regenerationRate, 0.0f, 1.0f);
        ImGui::SliderFloat("Scarcity", &r.scarcity, 0.0f, 1.0f);

        ImGui::Separator();
        ImGui::Text("Spawn Biomes");
        ImGui::Checkbox("Forest", &r.spawnsInForest);
        ImGui::Checkbox("Mountain", &r.spawnsInMountain);
        ImGui::Checkbox("Desert", &r.spawnsInDesert);
        ImGui::Checkbox("Ocean", &r.spawnsInOcean);
      }
      ImGui::EndGroup();

      ImGui::EndTabItem();
    }

    // --- TAB 2: AGENTS (Bio/Civ) ---
    if (ImGui::BeginTabItem("Agents")) {

      // LIST
      ImGui::BeginChild("AgentList", ImVec2(150, 0), ImGuiChildFlags_Borders);
      if (ImGui::Button("+ Add Agent")) {
        AssetManager::CreateNewAgent();
      }
      static int selectedAg = 0;
      for (int i = 0; i < (int)AssetManager::agentRegistry.size(); i++) {
        if (ImGui::Selectable(AssetManager::agentRegistry[i].name.c_str(),
                              selectedAg == i))
          selectedAg = i;
      }
      ImGui::EndChild();

      // DETAILS
      ImGui::SameLine();
      ImGui::BeginChild("AgentDetails");
      if (selectedAg < (int)AssetManager::agentRegistry.size()) {
        AgentDefinition &a = AssetManager::agentRegistry[selectedAg];

        // Name buffer for editing
        static char nameBuf[64];
        strncpy(nameBuf, a.name.c_str(), 63);
        if (ImGui::InputText("Name", nameBuf, 64)) {
          a.name = nameBuf;
        }

        // TYPE SELECTOR
        const char *types[] = {"Flora (Plant)", "Fauna (Animal)",
                               "Civ (Faction)"};
        int typeIdx = (int)a.type;
        if (ImGui::Combo("Class", &typeIdx, types, 3))
          a.type = (AgentType)typeIdx;

        ImGui::ColorEdit3("Map Color", a.color);

        ImGui::Separator();
        ImGui::Text("Biology - Temperature");
        ImGui::SliderFloat("Ideal Temp", &a.idealTemp, 0.0f, 1.0f);
        ImGui::SliderFloat("Deadly Low", &a.deadlyTempLow, 0.0f, 1.0f);
        ImGui::SliderFloat("Deadly High", &a.deadlyTempHigh, 0.0f, 1.0f);

        ImGui::Separator();
        ImGui::Text("Biology - Moisture");
        ImGui::SliderFloat("Ideal Moisture", &a.idealMoisture, 0.0f, 1.0f);
        ImGui::SliderFloat("Dead Dry", &a.deadlyMoistureLow, 0.0f, 1.0f);
        ImGui::SliderFloat("Dead Wet", &a.deadlyMoistureHigh, 0.0f, 1.0f);

        ImGui::Separator();
        ImGui::Text("Behavior");
        ImGui::SliderFloat("Resilience", &a.resilience, 0.0f, 1.0f);
        ImGui::SliderFloat("Aggression", &a.aggression, 0.0f, 1.0f);
        ImGui::SliderFloat("Expansion Rate", &a.expansionRate, 0.0f, 1.0f);
        ImGui::SliderFloat("Food Need/Tick", &a.foodRequirement, 0.0f, 10.0f);

        ImGui::Separator();
        ImGui::Text("Economy");
        DrawRequirementMap("Consumes (Diet)", a.diet);
        ImGui::Spacing();
        DrawRequirementMap("Produces (Output)", a.output);
      }
      ImGui::EndChild();

      ImGui::EndTabItem();
    }

    ImGui::EndTabBar();
  }

  // --- THE SAVE BUTTON ---
  ImGui::Separator();
  if (ImGui::Button("SAVE TO DISK (rules.json)", ImVec2(-1, 40))) {
    AssetManager::SaveAll();
  }

  ImGui::End();
}
