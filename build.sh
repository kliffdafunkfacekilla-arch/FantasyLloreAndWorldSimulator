#!/bin/bash
mkdir -p bin
mkdir -p build/platform build/io build/lore build/core build/visuals build/frontend build/biology build/environment build/simulation build/imgui build/apps
CXX="g++"
CXXFLAGS="-std=c++17 -Iinclude -Ideps/imgui -Ideps/imgui/backends -DGLEW_STATIC -O2"
LIBS="-lglfw -lGLEW -lGL"

$CXX $CXXFLAGS -c src/simulation/CivilizationSim.cpp -o build/simulation/CivilizationSim.o
$CXX $CXXFLAGS -c src/simulation/LogisticsSystem.cpp -o build/simulation/LogisticsSystem.o
$CXX $CXXFLAGS -c src/simulation/ConflictSystem.cpp -o build/simulation/ConflictSystem.o

echo "Built specific modules"
