@echo off
setlocal
cd /d "%~dp0"

:: Set Path to MinGW
set "PATH=C:\msys64\mingw64\bin;%PATH%"

:: Clean
if not exist bin mkdir bin
del /q bin\*.exe >nul 2>&1
echo [CLEAN] Removing old binaries...

:: Copy Assets
echo [COPY] Copying shaders...
xcopy /E /I /Y shaders bin\shaders >nul 2>&1

:: Build
echo [BUILD] Compiling Omnis Engine...
clang++ -std=c++20 ^
    src/main.cpp ^
    src/frontend/*.cpp ^
    src/terrain/*.cpp ^
    src/environment/*.cpp ^
    src/biology/*.cpp ^
    src/simulation/*.cpp ^
    src/lore/*.cpp ^
    deps/imgui/*.cpp ^
    deps/imgui/backends/imgui_impl_glfw.cpp ^
    deps/imgui/backends/imgui_impl_opengl3.cpp ^
    -I include ^
    -I deps/imgui ^
    -I deps/imgui/backends ^
    -I deps/glfw/include ^
    -I deps/glew/include ^
    -L C:/msys64/mingw64/lib ^
    -lglfw3 -lglew32 -lopengl32 -lgdi32 -luser32 -lkernel32 -lshell32 -lcomdlg32 ^
    -static-libgcc -static-libstdc++ ^
    -o bin/OmnisWorldEngine_v2.exe

if %ERRORLEVEL% EQU 0 (
    echo [SUCCESS] Build complete! Run bin\OmnisWorldEngine_v2.exe
    exit /b 0
) else (
    echo [ERROR] Build failed.
    exit /b 1
)
