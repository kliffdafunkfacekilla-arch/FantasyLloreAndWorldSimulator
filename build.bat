@echo off
setlocal EnableDelayedExpansion

echo [SETUP] Setting up environment...
set "PATH=C:\msys64\mingw64\bin;%PATH%"

:: Create Directories
if not exist bin mkdir bin
if not exist bin\data mkdir bin\data
if not exist src\apps mkdir src\apps
if not exist src\io mkdir src\io

:: Compiler Settings
set CXX=clang++
set CXXFLAGS=-std=c++17 -Iinclude -Ideps/imgui -Ideps/imgui/backends -Ideps/glew/include -Ideps/glfw/include -DGLEW_STATIC
set LIBS=-Lbin -L C:/msys64/mingw64/lib -lglew32 -lglfw3 -lopengl32 -lgdi32 -luser32 -lshell32

:: Common Source Files (Used by all apps)
set SRC_COMMON=src/platform/WindowsUtils.cpp src/io/PlatformUtils.cpp src/io/BinaryExporter.cpp src/io/AssetManager.cpp src/io/stb_image_impl.cpp deps/imgui/imgui.cpp deps/imgui/imgui_draw.cpp deps/imgui/imgui_tables.cpp deps/imgui/imgui_widgets.cpp deps/imgui/misc/cpp/imgui_stdlib.cpp deps/imgui/backends/imgui_impl_glfw.cpp deps/imgui/backends/imgui_impl_opengl3.cpp

:: --- APP 1: SAGA ARCHITECT (Map Builder) ---
echo [BUILD] S.A.G.A. Architect...
%CXX% %CXXFLAGS% ^
    src/apps/App_Architect.cpp ^
    src/core/TerrainController.cpp ^
    src/core/NeighborFinder.cpp ^
    src/visuals/MapRenderer.cpp ^
    src/frontend/GuiController.cpp ^
    src/environment/ClimateSim.cpp ^
    src/environment/HydrologySim.cpp ^
    src/io/HeightmapLoader.cpp ^
    src/lore/LoreScribe.cpp ^
    src/biology/AgentSystem.cpp ^
    src/environment/DisasterSystem.cpp ^
    src/lore/NameGenerator.cpp ^
    %SRC_COMMON% ^
    -o bin/SAGA_Architect.exe ^
    %LIBS%

if %errorlevel% neq 0 (
    echo [ERROR] SAGA Architect Build Failed.
    pause
    exit /b %errorlevel%
)

:: --- APP 2: SAGA DATABASE (Lore & Rules) ---
echo [BUILD] S.A.G.A. Database...
%CXX% %CXXFLAGS% ^
    src/apps/App_Lore.cpp ^
    src/frontend/EditorUI.cpp ^
    src/lore/NameGenerator.cpp ^
    src/lore/LoreScribe.cpp ^
    %SRC_COMMON% ^
    -o bin/SAGA_Database.exe ^
    %LIBS%

if %errorlevel% neq 0 (
    echo [ERROR] SAGA Database Build Failed.
    pause
    exit /b %errorlevel%
)

:: --- APP 3: SAGA ENGINE (Headless Simulator) ---
echo [BUILD] S.A.G.A. Engine...
%CXX% %CXXFLAGS% ^
    src/apps/App_Sim.cpp ^
    src/core/NeighborFinder.cpp ^
    src/biology/AgentSystem.cpp ^
    src/simulation/CivilizationSim.cpp ^
    src/simulation/ConflictSystem.cpp ^
    src/simulation/LogisticsSystem.cpp ^
    src/simulation/UnitSystem.cpp ^
    src/environment/ChaosField.cpp ^
    src/environment/DisasterSystem.cpp ^
    src/environment/ClimateSim.cpp ^
    src/environment/HydrologySim.cpp ^
    src/lore/LoreScribe.cpp ^
    src/lore/NameGenerator.cpp ^
    %SRC_COMMON% ^
    -o bin/SAGA_Engine.exe ^
    %LIBS%

if %errorlevel% neq 0 (
    echo [ERROR] SAGA Engine Build Failed.
    pause
    exit /b %errorlevel%
)

echo [SUCCESS] S.A.G.A. Suite Build Complete.
echo 1. bin/SAGA_Architect.exe (Build the World)
echo 2. bin/SAGA_Database.exe  (Write the Rules)
echo 3. bin/SAGA_Engine.exe    (Run the Simulation)
pause
