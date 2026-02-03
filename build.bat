@echo off
if not exist bin mkdir bin

echo Compiling with clang++...
clang++ -std=c++17 ^
    src/main.cpp ^
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
    src/io/HeightmapLoader.cpp ^
    src/io/BinaryExporter.cpp ^
    src/io/LoreScribe.cpp ^
    -o bin/OmnisWorldEngine.exe ^
    -I include ^
    -I src/core ^
    -I src/io ^
    -lgdi32 -luser32 -lkernel32 -lglfw3 -lglew32 -lopengl32

if %ERRORLEVEL% EQU 0 (
    echo Build Successful! Run with: bin\OmnisWorldEngine.exe
) else (
    echo Build Failed!
)
