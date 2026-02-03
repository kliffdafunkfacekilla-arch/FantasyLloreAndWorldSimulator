@echo off
if not exist bin mkdir bin

:: Auto-configure environment for MSYS2
if exist "C:\msys64\mingw64\bin" (
    echo [SETUP] Found MSYS2, adding to PATH...
    set "PATH=C:\msys64\mingw64\bin;%PATH%"
)

echo Compiling with clang++...
clang++ -std=c++17 ^
    src/main.cpp ^
    src/core/MemoryManager.cpp ^
    src/core/PointGenerator.cpp ^
    src/core/NeighborFinder.cpp ^
    src/core/TerrainController.cpp ^
    src/core/SimulationLoop.cpp ^
    src/core/Timelapse.cpp ^
    src/modules/ChaosField.cpp ^
    src/modules/HydrologySim.cpp ^
    src/modules/ClimateSim.cpp ^
    src/modules/LifeSim.cpp ^
    src/modules/AgentSystem.cpp ^
    src/modules/FactionSim.cpp ^
    src/modules/ChronosSystem.cpp ^
    src/modules/LogisticsSystem.cpp ^
    src/modules/ConflictSystem.cpp ^
    src/visuals/MapRenderer.cpp ^
    src/io/HeightmapLoader.cpp ^
    src/io/BinaryExporter.cpp ^
    src/io/LoreScribe.cpp ^
    deps/imgui/imgui.cpp ^
    deps/imgui/imgui_draw.cpp ^
    deps/imgui/imgui_tables.cpp ^
    deps/imgui/imgui_widgets.cpp ^
    deps/imgui/imgui_demo.cpp ^
    deps/imgui/backends/imgui_impl_glfw.cpp ^
    deps/imgui/backends/imgui_impl_opengl3.cpp ^
    -o bin/OmnisWorldEngine.exe ^
    -I include ^
    -I src/core ^
    -I src/io ^
    -I deps/imgui ^
    -I deps/imgui/backends ^
    -lglfw3 -lglew32 -lopengl32 -lgdi32 -luser32 -lkernel32 -limm32 -lshell32 

if %ERRORLEVEL% EQU 0 (
    echo.
    echo [SUCCESS] Build Complete! 
    echo Run the engine: bin\OmnisWorldEngine.exe
) else (
    echo.
    echo [ERROR] Build Failed!
    echo Ensure you have installed MSYS2 packages: clang, glfw, glew
)
