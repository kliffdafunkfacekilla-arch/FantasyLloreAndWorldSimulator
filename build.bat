@echo off
setlocal

:: 1. Add MSYS2 tools to PATH
set PATH=C:\msys64\mingw64\bin;%PATH%

:: 2. Check for Clang
where clang++ >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] clang++ not found! Ensure MSYS2 is installed and mingw-w64-x86_64-clang is installed.
    exit /b 1
)

echo [SETUP] Found MSYS2, adding to PATH...

:: 3. Define Sources Explicitly
:: CORE: Timelapse.cpp is here
set CORE_SRC=src/core/TerrainController.cpp src/core/NeighborFinder.cpp src/core/SimulationLoop.cpp src/core/MemoryManager.cpp src/core/Timelapse.cpp

:: VISUALS
set VISUALS_SRC=src/visuals/MapRenderer.cpp

:: MODULES: Exclude Timelapse (moved to Core)
set MODULES_SRC=src/modules/ClimateSim.cpp src/modules/AgentSystem.cpp src/modules/ConflictSystem.cpp src/modules/LifeSim.cpp src/modules/HydrologySim.cpp src/modules/FactionSim.cpp src/modules/ChronosSystem.cpp src/modules/LogisticsSystem.cpp src/modules/ChaosField.cpp

:: PLATFORM
set PLATFORM_SRC=src/platform/WindowsUtils.cpp

:: IMGUI: Only GLFW/OpenGL3 backends
set IMGUI_SRC=deps/imgui/imgui.cpp deps/imgui/imgui_draw.cpp deps/imgui/imgui_tables.cpp deps/imgui/imgui_widgets.cpp deps/imgui/backends/imgui_impl_glfw.cpp deps/imgui/backends/imgui_impl_opengl3.cpp

:: MAIN
set MAIN_SRC=src/main.cpp

set SOURCES=%MAIN_SRC% %CORE_SRC% %VISUALS_SRC% %MODULES_SRC% %PLATFORM_SRC% %IMGUI_SRC%
set OUTPUT=bin\OmnisWorldEngine.exe

:: 4. Libraries
set LIBS=-lglfw3 -lglew32 -lopengl32 -lgdi32 -limm32 -lshell32 -lcomdlg32

:: 5. Create bin directory
if not exist bin mkdir bin

:: 6. Compile
echo Compiling with clang++...
clang++ -std=c++17 -Wall -Wextra %SOURCES% -o %OUTPUT% -Iinclude -Ideps/imgui -Ideps/imgui/backends -Llib %LIBS%

if %ERRORLEVEL% EQU 0 (
    echo [SUCCESS] Build Complete: %OUTPUT%
    echo Run the engine: bin\OmnisWorldEngine.exe
) else (
    echo [ERROR] Build Failed!
    echo Check build_debug.txt for details if needed.
)

endlocal
