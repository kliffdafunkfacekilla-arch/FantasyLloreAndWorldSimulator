@echo off
setlocal EnableDelayedExpansion

echo [SETUP] Setting up environment...
set "PATH=C:\msys64\mingw64\bin;%PATH%"

if not exist bin mkdir bin
if not exist obj mkdir obj
del /q obj\*.o >nul 2>&1

echo [COPY] Copying shaders and data...
if not exist bin\shaders mkdir bin\shaders
xcopy /Y /I /E shaders bin\shaders > nul

:: --- COMMON SHARED MODULES ---
echo [COMPILE] Shared Modules...
clang++ -c src/frontend/PlatformUtils.cpp -I include -std=c++17 -o obj/PlatformUtils.o
clang++ -c src/simulation/AssetManager.cpp -I include -std=c++17 -o obj/AssetManager.o
clang++ -c src/simulation/BinaryExporter.cpp -I include -std=c++17 -o obj/BinaryExporter.o
clang++ -c src/lore/LoreScribe.cpp -I include -std=c++17 -o obj/LoreScribe.o
clang++ -c src/lore/NameGenerator.cpp -I include -std=c++17 -o obj/NameGenerator.o
clang++ -c src/simulation/NeighborFinder.cpp -I include -std=c++17 -o obj/NeighborFinder.o
clang++ -c src/environment/ClimateSim.cpp -I include -std=c++17 -o obj/ClimateSim.o
clang++ -c src/environment/HydrologySim.cpp -I include -std=c++17 -o obj/HydrologySim.o
clang++ -c src/environment/DisasterSystem.cpp -I include -std=c++17 -o obj/DisasterSystem.o
clang++ -c src/biology/AgentSystem.cpp -I include -std=c++17 -o obj/AgentSystem.o

:: --- IMGUI SHARED ---
echo [COMPILE] ImGui Modules...
clang++ -c deps/imgui/imgui.cpp -I deps/imgui -o obj/imgui.o
clang++ -c deps/imgui/imgui_draw.cpp -I deps/imgui -o obj/imgui_draw.o
clang++ -c deps/imgui/imgui_tables.cpp -I deps/imgui -o obj/imgui_tables.o
clang++ -c deps/imgui/imgui_widgets.cpp -I deps/imgui -o obj/imgui_widgets.o
clang++ -c deps/imgui/backends/imgui_impl_glfw.cpp -I deps/imgui -I deps/glfw/include -o obj/imgui_impl_glfw.o
clang++ -c deps/imgui/backends/imgui_impl_opengl3.cpp -I deps/imgui -I deps/glew/include -o obj/imgui_impl_opengl3.o

:: --- 1. BUILD ARCHITECT (Map Builder) ---
echo [BUILD] OmnisArchitect.exe...
clang++ -c src/apps/App_Architect.cpp -I include -I deps/imgui -I deps/imgui/backends -I deps/glew/include -I deps/glfw/include -std=c++17 -o obj/App_Architect.o
clang++ -c src/frontend/MapRenderer.cpp -I include -std=c++17 -o obj/MapRenderer.o
clang++ -c src/frontend/GuiController.cpp -I include -I deps/imgui -I deps/glfw/include -std=c++17 -o obj/GuiController.o
clang++ -c src/terrain/TerrainController.cpp -I include -std=c++17 -o obj/TerrainController.o
clang++ -c src/terrain/HeightmapLoader.cpp -I include -std=c++17 -o obj/HeightmapLoader.o

clang++ -o bin/OmnisArchitect.exe obj/App_Architect.o obj/MapRenderer.o obj/GuiController.o obj/TerrainController.o obj/HeightmapLoader.o obj/NeighborFinder.o obj/ClimateSim.o obj/HydrologySim.o obj/DisasterSystem.o obj/AgentSystem.o obj/NameGenerator.o obj/AssetManager.o obj/BinaryExporter.o obj/LoreScribe.o obj/PlatformUtils.o obj/imgui*.o -L C:/msys64/mingw64/lib -lglew32 -lglfw3 -lopengl32 -lgdi32 -luser32 -lshell32 -DGLEW_STATIC
if %ERRORLEVEL% NEQ 0 ( echo [ERROR] Architect Build Failed! & exit /b 1 )

:: --- 2. BUILD LORE KEEPER (Database Tool) ---
echo [BUILD] OmnisLoreKeeper.exe...
clang++ -c src/apps/App_Lore.cpp -I include -I deps/imgui -I deps/imgui/backends -I deps/glew/include -I deps/glfw/include -std=c++17 -o obj/App_Lore.o
clang++ -c src/frontend/EditorUI.cpp -I include -I deps/imgui -std=c++17 -o obj/EditorUI.o

clang++ -o bin/OmnisLoreKeeper.exe obj/App_Lore.o obj/EditorUI.o obj/AssetManager.o obj/LoreScribe.o obj/NameGenerator.o obj/PlatformUtils.o obj/imgui*.o -L C:/msys64/mingw64/lib -lglew32 -lglfw3 -lopengl32 -lgdi32 -luser32 -lshell32 -DGLEW_STATIC
if %ERRORLEVEL% NEQ 0 ( echo [ERROR] LoreKeeper Build Failed! & exit /b 1 )

:: --- 3. BUILD SIMULATOR (Engine) ---
echo [BUILD] OmnisSimulator.exe...
clang++ -c src/apps/App_Sim.cpp -I include -std=c++17 -o obj/App_Sim.o
clang++ -c src/simulation/CivilizationSim.cpp -I include -std=c++17 -o obj/CivilizationSim.o
clang++ -c src/simulation/ConflictSystem.cpp -I include -std=c++17 -o obj/ConflictSystem.o
clang++ -c src/simulation/LogisticsSystem.cpp -I include -std=c++17 -o obj/LogisticsSystem.o
clang++ -c src/simulation/UnitSystem.cpp -I include -std=c++17 -o obj/UnitSystem.o
clang++ -c src/environment/ChaosField.cpp -I include -std=c++17 -o obj/ChaosField.o

clang++ -o bin/OmnisSimulator.exe obj/App_Sim.o obj/AgentSystem.o obj/CivilizationSim.o obj/ConflictSystem.o obj/LogisticsSystem.o obj/UnitSystem.o obj/ChaosField.o obj/DisasterSystem.o obj/ClimateSim.o obj/HydrologySim.o obj/NeighborFinder.o obj/AssetManager.o obj/BinaryExporter.o obj/LoreScribe.o obj/NameGenerator.o
if %ERRORLEVEL% NEQ 0 ( echo [ERROR] Simulator Build Failed! & exit /b 1 )

echo [SUCCESS] Multitool Architecture Built Successfully!
pause
