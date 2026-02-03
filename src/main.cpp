#define GLEW_STATIC
#define GLFW_INCLUDE_NONE
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

// 1. Include Your Headers
#include "WorldEngine.hpp"
#include "SimulationModules.hpp"
#include "PlatformUtils.hpp" // Use the separate file for the File Browser

// Include your Visuals (If not using a header, we include the cpp for Unity Build)
#include "visuals/MapRenderer.cpp"

// --- Helper: Shader Loader (Keep this here for simplicity) ---
#include <fstream>
#include <sstream>
std::string ReadFile(const char* path) {
    std::ifstream file(path);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

GLuint LoadShaders() {
    // Ensure world.vert and world.frag are in your bin/ folder!
    std::string vertCode = ReadFile("world.vert");
    std::string fragCode = ReadFile("world.frag");

    if (vertCode.empty() || fragCode.empty()) {
        printf("[ERROR] Shaders not found!\n");
        return 0;
    }

    const char* vShaderCode = vertCode.c_str();
    const char* fShaderCode = fragCode.c_str();

    GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);

    GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);

    glDeleteShader(vertex);
    glDeleteShader(fragment);
    return program;
}

// --- The God Mode Dashboard ---
void DrawGodModeUI(WorldSettings& settings, WorldBuffers& buffers, TerrainController& terrain) {
    ImGui::Begin("God Mode Dashboard");

    // Perspective
    if (ImGui::CollapsingHeader("Perspective & Scale", ImGuiTreeNodeFlags_DefaultOpen)) {
        const char* zoomLevels[] = { "Global", "Quadrant", "Regional", "Local" };
        if (ImGui::Combo("View Level", &settings.zoomLevel, zoomLevels, 4)) {
            settings.pointSize = (settings.zoomLevel == 0) ? 1.0f : (settings.zoomLevel == 1) ? 4.0f : (settings.zoomLevel == 2) ? 16.0f : 64.0f;
        }
        ImGui::SliderFloat2("View Offset", settings.viewOffset, 0.0f, 1.0f);
    }

    // Terrain
    if (ImGui::CollapsingHeader("Terrain & Geology")) {
        if (ImGui::Button("Import Heightmap")) {
            // FIX #1: Call the function from PlatformUtils
            std::string path = PlatformUtils::OpenFileDialog();
            if (!path.empty()) terrain.LoadFromImage(path.c_str(), buffers);
        }
        ImGui::SliderFloat("Sea Level", &settings.seaLevel, 0.0f, 1.0f);
        ImGui::SliderFloat("Roughness", &settings.heightSeverity, 0.1f, 5.0f);

        if (ImGui::Button("Regenerate Procedural")) {
            terrain.GenerateProceduralTerrain(buffers, settings);
        }

        ImGui::Separator();
        ImGui::SliderInt("Erosion Passes", &settings.erosionIterations, 1, 100);
        if (ImGui::Button("Run Thermal Erosion")) {
             // Link to the Erosion Module
             terrain.ApplyThermalErosion(buffers, settings.erosionIterations);
        }
    }

    // Climate
    if (ImGui::CollapsingHeader("Atmosphere")) {
        ImGui::Checkbox("Manual Wind", &settings.manualWindOverride);
        if (settings.manualWindOverride) {
            ImGui::SliderFloat("N. Pole", &settings.windZones[0], -1.0f, 1.0f);
            ImGui::SliderFloat("Equator", &settings.windZones[2], -1.0f, 1.0f);
            ImGui::SliderFloat("S. Pole", &settings.windZones[4], -1.0f, 1.0f);
        }
    }

    ImGui::End();
}

int main() {
    if (!glfwInit()) return -1;
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Omnis World Engine", NULL, NULL);
    if (!window) return -1;
    glfwMakeContextCurrent(window);
    glewInit();

    // 2. Initialize Core Systems
    WorldBuffers buffers;
    buffers.Initialize(1000000);
    WorldSettings settings;

    // FIX #2: Instantiate the Controller
    TerrainController terrain;
    MapRenderer renderer;
    renderer.Setup(buffers);

    // FIX #3: Load Shaders
    GLuint shaderProgram = LoadShaders();

    // Generate Initial World
    terrain.GenerateProceduralTerrain(buffers, settings);

    // Setup ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // 5. Main Loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // UI
        ImGui_ImplOpenGL3_NewFrame(); ImGui_ImplGlfw_NewFrame(); ImGui::NewFrame();
        DrawGodModeUI(settings, buffers, terrain);

        // Simulation
        ClimateSim::Update(buffers, settings);

        // Render
        int w, h; glfwGetFramebufferSize(window, &w, &h);
        glViewport(0, 0, w, h);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Activate Shaders and Render
        glUseProgram(shaderProgram);

        // Update Uniforms
        glUniform1f(glGetUniformLocation(shaderProgram, "u_zoom"), (float)pow(4.0, settings.zoomLevel));
        glUniform2f(glGetUniformLocation(shaderProgram, "u_offset"), settings.viewOffset[0], settings.viewOffset[1]);
        glUniform1f(glGetUniformLocation(shaderProgram, "u_pointSize"), settings.pointSize);

        renderer.UpdateVisuals(buffers, settings);
        renderer.Render(settings);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown(); ImGui_ImplGlfw_Shutdown(); ImGui::DestroyContext();
    glfwDestroyWindow(window); glfwTerminate();
    return 0;
}
