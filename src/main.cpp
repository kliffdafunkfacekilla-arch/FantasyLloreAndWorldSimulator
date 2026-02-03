#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
// Placeholder for ImGui
// #include "imgui.h" 

#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>

// Core Engine Headers
#include "../include/WorldEngine.hpp"
#include "../include/SimulationModules.hpp"

#include "visuals/MapRenderer.cpp"
#include "core/MemoryManager.cpp"
#include "../src/core/FastNoiseLite.h"

// --- 1. Consolidated Terrain Generation (FastNoiseLite) ---
void GenerateTerrain(WorldBuffers& b, const WorldSettings& s) {
    FastNoiseLite noise;
    noise.SetSeed(s.seed);
    noise.SetFrequency(s.featureFrequency);
    noise.SetFractalLacunarity(s.featureClustering);
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);

    int side = (int)std::sqrt(b.count);

    for (uint32_t i = 0; i < b.count; ++i) {
        if(b.posX && b.posY) {
            b.posX[i] = (float)(i % side) / side;
            b.posY[i] = (float)(i / side) / side;
        }
        float noiseVal = noise.GetNoise(b.posX[i] * 100.0f, b.posY[i] * 100.0f);
        float h = std::pow(std::abs(noiseVal), s.heightSeverity);
        h *= s.heightMultiplier;
        b.height[i] = std::clamp(h, s.heightMin, s.heightMax);
    }
}

// --- 2. 5-Zone Climate & Wind Logic ---
void UpdateGlobalClimate(WorldBuffers& b, const WorldSettings& s) {
    for (uint32_t i = 0; i < b.count; ++i) {
        float y = b.posY[i]; 
        float latEffect = (s.worldScaleMode == 3) ? (1.0f - std::abs(y - 0.5f) * 2.0f) : (1.0f - y);
        float elevationChill = b.height[i] * 0.5f;
        if (b.temperature) b.temperature[i] = (latEffect - elevationChill) * s.globalTempModifier;

        if (b.windDX) {
            if (y < 0.2f || (y >= 0.4f && y < 0.6f) || y >= 0.8f) {
                b.windDX[i] = -1.0f * s.globalWindStrength; 
            } else {
                b.windDX[i] = 1.0f * s.globalWindStrength;  
            }
        }
    }
}

// --- 3. Visualizer Bridge ---
void UpdateVisualBuffers(WorldBuffers& b, const WorldSettings& s, MapRenderer& r) {
    for(uint32_t i=0; i<b.count; ++i) {
        uint32_t idx = i * 3;
        float h = b.height[i];

        if (h < s.seaLevel) {
            r.colorBuffer[idx+0] = 0.1f; r.colorBuffer[idx+1] = 0.2f; r.colorBuffer[idx+2] = 0.6f;
        } else if (b.temperature && b.temperature[i] < 0.2f) {
            r.colorBuffer[idx+0] = 0.9f; r.colorBuffer[idx+1] = 0.9f; r.colorBuffer[idx+2] = 1.0f;
        } else {
            if (b.factionID && b.factionID[i] > 0) {
                 // Simple red tint for Factions
                 r.colorBuffer[idx+0] = 0.8f; 
                 r.colorBuffer[idx+1] = 0.2f; 
                 r.colorBuffer[idx+2] = 0.2f;
            } else {
                 r.colorBuffer[idx+0] = 0.2f; r.colorBuffer[idx+1] = 0.5f + (h * 0.2f); r.colorBuffer[idx+2] = 0.2f;
            }
        }
    }
    r.UploadColors(b.count);
}

// --- UI Stub ---
void DrawLoreUI(LoreScribe& scribe, TimelapseManager& tl, uint32_t currentCenterCell) {
    // Requires ImGui. Code commented to avoid build error but structure preserved.
    /*
    ImGui::Begin("Lore & History");
    if (ImGui::Button("Tag Current View for Timelapse")) {
        tl.TagRegion(currentCenterCell, 50);
    }
    ImGui::Separator();
    ImGui::Text("Recent History:");
    for (auto& event : scribe.GetSessionLogs()) {
        ImGui::Text("[%d/%d/%d] %s: %s", 
            event.year, event.month, event.day, 
            event.category.c_str(), event.log.c_str());
    }
    ImGui::End();
    */
    
    // Simple Console Dump substitute
    static size_t lastSize = 0;
    auto& logs = scribe.GetSessionLogs();
    if (logs.size() > lastSize) {
        // Print new logs to console
        auto& lat = logs.back();
        // std::cout << "UI: [" << lat.year << "] " << lat.log << "\n";
        lastSize = logs.size();
    }
}

int main() {
    // 1. Setup Data
    WorldBuffers buffers;
    WorldSettings settings;
    MemoryManager mem;
    mem.InitializeWorld(settings, buffers); 

    // 2. Generation Phase
    GenerateTerrain(buffers, settings);
    
    NeighborGraph graph;
    NeighborFinder finder;
    finder.BuildGraph(buffers, settings.cellCount, graph);
    
    UpdateGlobalClimate(buffers, settings);

    // 3. Visualization Setup
    if (!glfwInit()) return -1;
    GLFWwindow* window = glfwCreateWindow(1280, 720, "OMNIS WORLD ENGINE", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    if (glewInit() != GLEW_OK) return -1;

    MapRenderer renderer;
    renderer.Setup(buffers);

    // 4. Chronos & Simulation
    ChronosConfig clock;
    SimulationLoop simLoop;
    simLoop.settings = settings;
    simLoop.scribe.Initialize("OmnisWorld");

    std::vector<AgentTemplate> registry; 
    std::vector<Faction> factions;       
    Faction f1; f1.id = 1; f1.name = "Empire";
    factions.push_back(f1);
    
    // 5. Main Loop
    while (!glfwWindowShouldClose(window)) {
        
        simLoop.Update(buffers, registry, factions, clock, graph, 0.016f);
        
        // Manual updates for test
        clock.dayCount++; 
        // Logic for month/year rollover handled in ChronosSystem typically
        // but here simplified.
        
        UpdateVisualBuffers(buffers, settings, renderer);
        DrawLoreUI(simLoop.scribe, simLoop.timelapse, 500000); // Center

        glClear(GL_COLOR_BUFFER_BIT);
        renderer.Render(buffers);
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    finder.Cleanup(graph);
    glfwTerminate();
    buffers.clear();
    return 0;
}
