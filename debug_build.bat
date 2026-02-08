@echo off
set "PATH=C:\msys64\mingw64\bin;%PATH%"

echo [DEBUG] Compiling ConflictSystem...
clang++ -c src/simulation/ConflictSystem.cpp -I include -std=c++17 -o conflict_test.o > conflict_debug.log 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] ConflictSystem failed!
    type conflict_debug.log
    exit /b 1
)

echo [DEBUG] Compiling DisasterSystem...
clang++ -c src/environment/DisasterSystem.cpp -I include -std=c++17 -o disaster_test.o > disaster_debug.log 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] DisasterSystem failed!
    type disaster_debug.log
    exit /b 1
)

echo [DEBUG] Compiling Main...
clang++ -c src/main.cpp -I include -I deps/imgui -I deps/imgui/backends -I deps/glew/include -I deps/glfw/include -std=c++17 -o main_test.o > main_debug.log 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Main failed!
    type main_debug.log
    exit /b 1
)

echo [DEBUG] Compiling CivilizationSim...
clang++ -c src/simulation/CivilizationSim.cpp -I include -std=c++17 -o civ_test.o > civ_debug.log 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] CivilizationSim failed!
    type civ_debug.log
    exit /b 1
)

echo [DEBUG] Compiling ClimateSim...
clang++ -c src/environment/ClimateSim.cpp -I include -std=c++17 -o climate_test.o > climate_debug.log 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] ClimateSim failed!
    type climate_debug.log
    exit /b 1
)

echo [DEBUG] Compiling HydrologySim...
clang++ -c src/environment/HydrologySim.cpp -I include -std=c++17 -o hydro_test.o > hydro_debug.log 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] HydrologySim failed!
    type hydro_debug.log
    exit /b 1
)

echo [DEBUG] Compiling GuiController...
clang++ -c src/frontend/GuiController.cpp -I include -I deps/imgui -I deps/imgui/backends -I deps/glfw/include -std=c++17 -o gui_test.o > gui_debug.log 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] GuiController failed!
    type gui_debug.log
    exit /b 1
)

echo [SUCCESS] All key files compiled.
