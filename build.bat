@echo off
set "PATH=C:\msys64\mingw64\bin;%PATH%"
if not exist bin mkdir bin

echo [CLEAN] Removing old binaries...
del /q bin\*.exe
echo [COPY] Copying shaders...
copy /y shaders\* bin\

echo [BUILD] Compiling Omnis Engine...
clang++ -std=c++20 ^
    src/main.cpp ^
    src/core/*.cpp ^
    src/modules/*.cpp ^
    src/io/*.cpp ^
    src/ui/*.cpp ^
    deps/imgui/*.cpp ^
    deps/imgui/backends/imgui_impl_glfw.cpp ^
    deps/imgui/backends/imgui_impl_opengl3.cpp ^
    -I include -I deps/imgui -I deps/imgui/backends -I deps/glfw/include -I deps/glew/include ^
    -L C:/msys64/mingw64/lib ^
    -lglfw3 -lglew32 -lopengl32 -lgdi32 -luser32 -lkernel32 -lshell32 -lcomdlg32 ^
    -static-libgcc -static-libstdc++ ^
    -o bin/OmnisWorldEngine.exe

if %ERRORLEVEL% EQU 0 (
    echo [SUCCESS] Engine built successfully.
    echo [RUN] Launching...
    bin\OmnisWorldEngine.exe
) else (
    echo [ERROR] Build failed. Check the logs above.
    pause
)
