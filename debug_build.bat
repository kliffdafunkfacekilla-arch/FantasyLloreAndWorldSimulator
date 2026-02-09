@echo off
set "PATH=C:\msys64\mingw64\bin;%PATH%"
clang++ -c src/apps/App_Architect.cpp -I include -I deps/imgui -I deps/imgui/backends -I deps/glew/include -I deps/glfw/include -std=c++17 -o obj/App_Architect.o 2> build_error.txt
type build_error.txt
