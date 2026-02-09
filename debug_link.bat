@echo off
set "PATH=C:\msys64\mingw64\bin;%PATH%"
clang++ -o bin/OmnisArchitect.exe obj/App_Architect.o obj/MapRenderer.o obj/GuiController.o obj/TerrainController.o obj/HeightmapLoader.o obj/NeighborFinder.o obj/ClimateSim.o obj/HydrologySim.o obj/AssetManager.o obj/BinaryExporter.o obj/LoreScribe.o obj/PlatformUtils.o obj/imgui*.o -L C:/msys64/mingw64/lib -lglew32 -lglfw3 -lopengl32 -lgdi32 -luser32 -lshell32 -DGLEW_STATIC 2> link_error.txt
type link_error.txt
