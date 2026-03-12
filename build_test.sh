#!/bin/bash
mkdir -p bin
mkdir -p build/platform build/io build/lore build/core build/visuals build/frontend build/biology build/environment build/simulation build/imgui build/apps

CXX="g++"
CXXFLAGS="-std=c++17 -Iinclude -Ideps/imgui -Ideps/imgui/backends -DGLEW_STATIC -O2"
LIBS="-lglfw -lGLEW -lGL"

$CXX $CXXFLAGS -c src/platform/WindowsUtils.cpp -o build/platform/WindowsUtils.o
$CXX $CXXFLAGS -c src/io/PlatformUtils.cpp -o build/io/PlatformUtils.o
$CXX $CXXFLAGS -c src/io/BinaryExporter.cpp -o build/io/BinaryExporter.o
$CXX $CXXFLAGS -c src/io/AssetManager.cpp -o build/io/AssetManager.o
$CXX $CXXFLAGS -c src/io/LoreManager.cpp -o build/io/LoreManager.o
$CXX $CXXFLAGS -c src/io/stb_image_impl.cpp -o build/io/stb_image_impl.o
$CXX $CXXFLAGS -c src/io/HeightmapLoader.cpp -o build/io/HeightmapLoader.o

$CXX $CXXFLAGS -c src/lore/LoreScribe.cpp -o build/lore/LoreScribe.o
$CXX $CXXFLAGS -c src/lore/NameGenerator.cpp -o build/lore/NameGenerator.o

$CXX $CXXFLAGS -c src/core/TerrainController.cpp -o build/core/TerrainController.o
$CXX $CXXFLAGS -c src/core/NeighborFinder.cpp -o build/core/NeighborFinder.o

$CXX $CXXFLAGS -c src/visuals/MapRenderer.cpp -o build/visuals/MapRenderer.o
$CXX $CXXFLAGS -c src/frontend/GuiController.cpp -o build/frontend/GuiController.o
$CXX $CXXFLAGS -c src/frontend/EditorUI.cpp -o build/frontend/EditorUI.o
$CXX $CXXFLAGS -c src/frontend/WikiEditor.cpp -o build/frontend/WikiEditor.o

$CXX $CXXFLAGS -c src/biology/AgentSystem.cpp -o build/biology/AgentSystem.o

$CXX $CXXFLAGS -c src/environment/ClimateSim.cpp -o build/environment/ClimateSim.o
$CXX $CXXFLAGS -c src/environment/HydrologySim.cpp -o build/environment/HydrologySim.o
$CXX $CXXFLAGS -c src/environment/ChaosField.cpp -o build/environment/ChaosField.o
$CXX $CXXFLAGS -c src/environment/DisasterSystem.cpp -o build/environment/DisasterSystem.o

$CXX $CXXFLAGS -c src/simulation/CivilizationSim.cpp -o build/simulation/CivilizationSim.o
$CXX $CXXFLAGS -c src/simulation/ConflictSystem.cpp -o build/simulation/ConflictSystem.o
$CXX $CXXFLAGS -c src/simulation/LogisticsSystem.cpp -o build/simulation/LogisticsSystem.o
$CXX $CXXFLAGS -c src/simulation/UnitSystem.cpp -o build/simulation/UnitSystem.o

$CXX $CXXFLAGS -c deps/imgui/imgui.cpp -o build/imgui/imgui.o
$CXX $CXXFLAGS -c deps/imgui/imgui_draw.cpp -o build/imgui/imgui_draw.o
$CXX $CXXFLAGS -c deps/imgui/imgui_tables.cpp -o build/imgui/imgui_tables.o
$CXX $CXXFLAGS -c deps/imgui/imgui_widgets.cpp -o build/imgui/imgui_widgets.o
$CXX $CXXFLAGS -c deps/imgui/misc/cpp/imgui_stdlib.cpp -o build/imgui/imgui_stdlib.o
$CXX $CXXFLAGS -c deps/imgui/backends/imgui_impl_glfw.cpp -o build/imgui/imgui_impl_glfw.o
$CXX $CXXFLAGS -c deps/imgui/backends/imgui_impl_opengl3.cpp -o build/imgui/imgui_impl_opengl3.o

$CXX $CXXFLAGS -c src/apps/App_Sim.cpp -o build/apps/App_Sim.o

echo "Linking Engine..."
$CXX build/apps/App_Sim.o build/core/NeighborFinder.o build/biology/AgentSystem.o build/simulation/CivilizationSim.o build/simulation/ConflictSystem.o build/simulation/LogisticsSystem.o build/simulation/UnitSystem.o build/environment/ChaosField.o build/environment/DisasterSystem.o build/environment/ClimateSim.o build/environment/HydrologySim.o build/platform/WindowsUtils.o build/io/PlatformUtils.o build/io/BinaryExporter.o build/io/AssetManager.o build/io/LoreManager.o build/io/stb_image_impl.o build/lore/LoreScribe.o build/lore/NameGenerator.o build/imgui/imgui.o build/imgui/imgui_draw.o build/imgui/imgui_tables.o build/imgui/imgui_widgets.o build/imgui/imgui_stdlib.o build/imgui/imgui_impl_glfw.o build/imgui/imgui_impl_opengl3.o build/frontend/WikiEditor.o -o bin/SAGA_Engine $LIBS

if [ $? -eq 0 ]; then
    echo "Engine linked successfully!"
else
    echo "Link failed."
fi
