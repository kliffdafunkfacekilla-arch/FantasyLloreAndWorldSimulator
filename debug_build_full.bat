@echo off
set "PATH=C:\msys64\mingw64\bin;%PATH%"

mkdir obj 2>nul

echo [COMPILE] Main...
clang++ -c src/main.cpp -I include -I deps/imgui -I deps/imgui/backends -I deps/glew/include -I deps/glfw/include -std=c++17 -o obj/main.o
if %ERRORLEVEL% NEQ 0 exit /b 1

echo [COMPILE] Frontend...
clang++ -c src/frontend/PlatformUtils.cpp -I include -std=c++17 -o obj/PlatformUtils.o
if %ERRORLEVEL% NEQ 0 exit /b 1
clang++ -c src/frontend/MapRenderer.cpp -I include -std=c++17 -o obj/MapRenderer.o
if %ERRORLEVEL% NEQ 0 exit /b 1
clang++ -c src/frontend/GuiController.cpp -I include -I deps/imgui -I deps/imgui/backends -I deps/glfw/include -std=c++17 -o obj/GuiController.o
if %ERRORLEVEL% NEQ 0 exit /b 1
clang++ -c src/frontend/EditorUI.cpp -I include -I deps/imgui -std=c++17 -o obj/EditorUI.o
if %ERRORLEVEL% NEQ 0 exit /b 1

echo [COMPILE] Simulation...
clang++ -c src/simulation/AssetManager.cpp -I include -std=c++17 -o obj/AssetManager.o
if %ERRORLEVEL% NEQ 0 exit /b 1
clang++ -c src/simulation/CivilizationSim.cpp -I include -std=c++17 -o obj/CivilizationSim.o
if %ERRORLEVEL% NEQ 0 exit /b 1
clang++ -c src/simulation/ConflictSystem.cpp -I include -std=c++17 -o obj/ConflictSystem.o
if %ERRORLEVEL% NEQ 0 exit /b 1
clang++ -c src/simulation/LogisticsSystem.cpp -I include -std=c++17 -o obj/LogisticsSystem.o
if %ERRORLEVEL% NEQ 0 exit /b 1
clang++ -c src/simulation/UnitSystem.cpp -I include -std=c++17 -o obj/UnitSystem.o
if %ERRORLEVEL% NEQ 0 exit /b 1
clang++ -c src/simulation/NeighborFinder.cpp -I include -std=c++17 -o obj/NeighborFinder.o
if %ERRORLEVEL% NEQ 0 exit /b 1
clang++ -c src/simulation/BinaryExporter.cpp -I include -std=c++17 -o obj/BinaryExporter.o
if %ERRORLEVEL% NEQ 0 exit /b 1

echo [COMPILE] Terrain...
clang++ -c src/terrain/TerrainController.cpp -I include -std=c++17 -o obj/TerrainController.o
if %ERRORLEVEL% NEQ 0 exit /b 1
clang++ -c src/terrain/HeightmapLoader.cpp -I include -std=c++17 -o obj/HeightmapLoader.o
if %ERRORLEVEL% NEQ 0 exit /b 1

echo [COMPILE] Environment...
clang++ -c src/environment/ClimateSim.cpp -I include -std=c++17 -o obj/ClimateSim.o
if %ERRORLEVEL% NEQ 0 exit /b 1
clang++ -c src/environment/HydrologySim.cpp -I include -std=c++17 -o obj/HydrologySim.o
if %ERRORLEVEL% NEQ 0 exit /b 1
clang++ -c src/environment/ChaosField.cpp -I include -std=c++17 -o obj/ChaosField.o
if %ERRORLEVEL% NEQ 0 exit /b 1
clang++ -c src/environment/DisasterSystem.cpp -I include -std=c++17 -o obj/DisasterSystem.o
if %ERRORLEVEL% NEQ 0 exit /b 1

echo [COMPILE] Biology...
clang++ -c src/biology/AgentSystem.cpp -I include -std=c++17 -o obj/AgentSystem.o
if %ERRORLEVEL% NEQ 0 exit /b 1

echo [COMPILE] Lore...
clang++ -c src/lore/LoreScribe.cpp -I include -std=c++17 -o obj/LoreScribe.o
if %ERRORLEVEL% NEQ 0 exit /b 1
clang++ -c src/lore/NameGenerator.cpp -I include -std=c++17 -o obj/NameGenerator.o
if %ERRORLEVEL% NEQ 0 exit /b 1

echo [COMPILE] ImGui...
clang++ -c deps/imgui/imgui.cpp -I deps/imgui -o obj/imgui.o
clang++ -c deps/imgui/imgui_draw.cpp -I deps/imgui -o obj/imgui_draw.o
clang++ -c deps/imgui/imgui_tables.cpp -I deps/imgui -o obj/imgui_tables.o
clang++ -c deps/imgui/imgui_widgets.cpp -I deps/imgui -o obj/imgui_widgets.o
clang++ -c deps/imgui/backends/imgui_impl_glfw.cpp -I deps/imgui -I deps/glfw/include -o obj/imgui_impl_glfw.o
clang++ -c deps/imgui/backends/imgui_impl_opengl3.cpp -I deps/imgui -I deps/glew/include -o obj/imgui_impl_opengl3.o

echo [LINK] Linking...
clang++ -g -std=c++17 -o bin/OmnisWorldEngine_v3.exe obj/*.o -Ldeps/glew/lib/Release/x64 -Ldeps/glfw/lib-vc2022 -lglew32s -lglfw3 -lopengl32 -lgdi32 -luser32 -lshell32 -DGLEW_STATIC

if %ERRORLEVEL% EQU 0 (
    echo [SUCCESS] Build complete!
) else (
    echo [ERROR] Link failed!
)
