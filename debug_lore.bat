@echo off
set "PATH=C:\msys64\mingw64\bin;%PATH%"
clang++ -c src/apps/App_Lore.cpp -I include -I deps/imgui -I deps/imgui/backends -I deps/glew/include -I deps/glfw/include -std=c++17 -o obj/App_Lore.o 2> lore_error.txt
type lore_error.txt
