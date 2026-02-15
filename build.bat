@echo off
setlocal EnableDelayedExpansion

:: ============================================================
:: T.A.L.E.W.E.A.V.E.R.S. Incremental Build System
:: Compiles .cpp -> .o (cached), then links per-app.
:: Usage: build.bat          (incremental build all)
::        build.bat clean    (wipe object cache)
::        build.bat engine   (build only Engine)
::        build.bat arch     (build only Architect)
::        build.bat db       (build only Database)
::        build.bat proj     (build only Projector)
::        build.bat launch   (build only Launcher)
:: ============================================================

echo [SETUP] Setting up environment...
set "PATH=C:\msys64\mingw64\bin;%PATH%"

:: Global Data Hub Setup
set "DATA_HUB=C:\Users\krazy\Documents\GitHub\SAGA_Global_Data"
if not exist "%DATA_HUB%" mkdir "%DATA_HUB%"
if not exist "%DATA_HUB%\history" mkdir "%DATA_HUB%\history"
if not exist "%DATA_HUB%\sessions" mkdir "%DATA_HUB%\sessions"
if not exist "%DATA_HUB%\tactical_data" mkdir "%DATA_HUB%\tactical_data"

:: Create Directories
if not exist bin mkdir bin
for %%D in (apps core io biology simulation environment lore frontend visuals platform imgui) do (
    if not exist "build\%%D" mkdir "build\%%D"
)

:: Handle "clean" argument
if "%1"=="clean" (
    echo [CLEAN] Wiping build cache...
    if exist build rmdir /s /q build
    echo [CLEAN] Done. Run build.bat again to rebuild.
    exit /b 0
)

:: Compiler Settings
set CXX=clang++
set CXXFLAGS=-std=c++17 -Iinclude -Ideps/imgui -Ideps/imgui/backends -IC:/msys64/mingw64/include -DGLEW_STATIC
set LIBS=-Lbin -LC:/msys64/mingw64/lib -lglew32 -lglfw3 -lopengl32 -lgdi32 -luser32 -lshell32 -lcomdlg32 -lole32

:: Build target
set "TGT=%1"
if "%TGT%"=="" set "TGT=all"

:: ============================================================
:: STEP 1: Compile .cpp -> .o (skip if unchanged)
:: Strategy: After compiling, we write the source file size to 
:: a .stamp file. On next build, if size matches AND .o exists,
:: we skip. This is fast and reliable for batch.
:: ============================================================
set BUILD_ERRORS=0
set COMPILED_COUNT=0
set SKIPPED_COUNT=0

call :cc "src\platform\WindowsUtils.cpp"       "build\platform\WindowsUtils.o"
call :cc "src\io\PlatformUtils.cpp"            "build\io\PlatformUtils.o"
call :cc "src\io\BinaryExporter.cpp"           "build\io\BinaryExporter.o"
call :cc "src\io\AssetManager.cpp"             "build\io\AssetManager.o"
call :cc "src\io\LoreManager.cpp"              "build\io\LoreManager.o"
call :cc "src\io\stb_image_impl.cpp"           "build\io\stb_image_impl.o"
call :cc "src\io\HeightmapLoader.cpp"          "build\io\HeightmapLoader.o"
call :cc "src\lore\LoreScribe.cpp"             "build\lore\LoreScribe.o"
call :cc "src\lore\NameGenerator.cpp"          "build\lore\NameGenerator.o"
call :cc "src\core\TerrainController.cpp"      "build\core\TerrainController.o"
call :cc "src\core\NeighborFinder.cpp"         "build\core\NeighborFinder.o"
call :cc "src\visuals\MapRenderer.cpp"         "build\visuals\MapRenderer.o"
call :cc "src\frontend\GuiController.cpp"      "build\frontend\GuiController.o"
call :cc "src\frontend\EditorUI.cpp"           "build\frontend\EditorUI.o"
call :cc "src\biology\AgentSystem.cpp"         "build\biology\AgentSystem.o"
call :cc "src\environment\ClimateSim.cpp"      "build\environment\ClimateSim.o"
call :cc "src\environment\HydrologySim.cpp"    "build\environment\HydrologySim.o"
call :cc "src\environment\ChaosField.cpp"      "build\environment\ChaosField.o"
call :cc "src\environment\DisasterSystem.cpp"  "build\environment\DisasterSystem.o"
call :cc "src\simulation\CivilizationSim.cpp"  "build\simulation\CivilizationSim.o"
call :cc "src\simulation\ConflictSystem.cpp"   "build\simulation\ConflictSystem.o"
call :cc "src\simulation\LogisticsSystem.cpp"  "build\simulation\LogisticsSystem.o"
call :cc "src\simulation\UnitSystem.cpp"       "build\simulation\UnitSystem.o"
call :cc "deps\imgui\imgui.cpp"                        "build\imgui\imgui.o"
call :cc "deps\imgui\imgui_draw.cpp"                   "build\imgui\imgui_draw.o"
call :cc "deps\imgui\imgui_tables.cpp"                 "build\imgui\imgui_tables.o"
call :cc "deps\imgui\imgui_widgets.cpp"                "build\imgui\imgui_widgets.o"
call :cc "deps\imgui\misc\cpp\imgui_stdlib.cpp"        "build\imgui\imgui_stdlib.o"
call :cc "deps\imgui\backends\imgui_impl_glfw.cpp"     "build\imgui\imgui_impl_glfw.o"
call :cc "deps\imgui\backends\imgui_impl_opengl3.cpp"  "build\imgui\imgui_impl_opengl3.o"
call :cc "src\apps\App_Launcher.cpp"     "build\apps\App_Launcher.o"
call :cc "src\apps\App_Architect.cpp"    "build\apps\App_Architect.o"
call :cc "src\apps\App_Lore.cpp"         "build\apps\App_Lore.o"
call :cc "src\apps\App_Sim.cpp"          "build\apps\App_Sim.o"
call :cc "src\apps\App_Replay.cpp"       "build\apps\App_Replay.o"

echo.
echo [INFO] Compiled: !COMPILED_COUNT! ^| Cached: !SKIPPED_COUNT!

if !BUILD_ERRORS! neq 0 (
    echo [ERROR] !BUILD_ERRORS! file^(s^) failed to compile.
    exit /b 1
)

:: ============================================================
:: STEP 2: Link Executables
:: ============================================================
set OBJ_COMMON=build\platform\WindowsUtils.o build\io\PlatformUtils.o build\io\BinaryExporter.o build\io\AssetManager.o build\io\LoreManager.o build\io\stb_image_impl.o build\lore\LoreScribe.o build\lore\NameGenerator.o build\imgui\imgui.o build\imgui\imgui_draw.o build\imgui\imgui_tables.o build\imgui\imgui_widgets.o build\imgui\imgui_stdlib.o build\imgui\imgui_impl_glfw.o build\imgui\imgui_impl_opengl3.o

if "%TGT%"=="all" call :link_launcher
if "%TGT%"=="launch" call :link_launcher
if "%TGT%"=="all" call :link_arch
if "%TGT%"=="arch" call :link_arch
if "%TGT%"=="all" call :link_db
if "%TGT%"=="db" call :link_db
if "%TGT%"=="all" call :link_engine
if "%TGT%"=="engine" call :link_engine
if "%TGT%"=="all" call :link_proj
if "%TGT%"=="proj" call :link_proj

echo.
echo [SUCCESS] T.A.L.E.W.E.A.V.E.R.S. Build Complete.
echo 0. bin\TALEWEAVERS_Launcher.exe  (Central Dashboard)
echo 1. bin\TALEWEAVERS_Architect.exe (Build the World)
echo 2. bin\TALEWEAVERS_Database.exe  (Write the Rules)
echo 3. bin\TALEWEAVERS_Engine.exe    (Run the Simulation)
echo 4. bin\TALEWEAVERS_Projector.exe (Watch the History)
exit /b 0

:: ============================================================
:: LINK FUNCTIONS
:: ============================================================
:link_launcher
echo [LINK] Launcher...
%CXX% build\apps\App_Launcher.o %OBJ_COMMON% -o bin\TALEWEAVERS_Launcher.exe %LIBS%
if !errorlevel! neq 0 ( echo [ERROR] Launcher link failed. & exit /b 1 )
goto :eof

:link_arch
echo [LINK] Architect...
%CXX% build\apps\App_Architect.o build\core\TerrainController.o build\core\NeighborFinder.o build\visuals\MapRenderer.o build\frontend\GuiController.o build\environment\ClimateSim.o build\environment\HydrologySim.o build\io\HeightmapLoader.o build\biology\AgentSystem.o build\environment\DisasterSystem.o %OBJ_COMMON% -o bin\TALEWEAVERS_Architect.exe %LIBS%
if !errorlevel! neq 0 ( echo [ERROR] Architect link failed. & exit /b 1 )
goto :eof

:link_db
echo [LINK] Database...
%CXX% build\apps\App_Lore.o build\frontend\EditorUI.o %OBJ_COMMON% -o bin\TALEWEAVERS_Database.exe %LIBS%
if !errorlevel! neq 0 ( echo [ERROR] Database link failed. & exit /b 1 )
goto :eof

:link_engine
echo [LINK] Engine...
%CXX% build\apps\App_Sim.o build\core\NeighborFinder.o build\biology\AgentSystem.o build\simulation\CivilizationSim.o build\simulation\ConflictSystem.o build\simulation\LogisticsSystem.o build\simulation\UnitSystem.o build\environment\ChaosField.o build\environment\DisasterSystem.o build\environment\ClimateSim.o build\environment\HydrologySim.o %OBJ_COMMON% -o bin\TALEWEAVERS_Engine.exe %LIBS%
if !errorlevel! neq 0 ( echo [ERROR] Engine link failed. & exit /b 1 )
goto :eof

:link_proj
echo [LINK] Projector...
%CXX% build\apps\App_Replay.o %OBJ_COMMON% -o bin\TALEWEAVERS_Projector.exe %LIBS%
if !errorlevel! neq 0 ( echo [ERROR] Projector link failed. & exit /b 1 )
goto :eof

:: ============================================================
:: COMPILE FUNCTION
:: Compiles SRC to OBJ. Skips if OBJ exists and source size
:: matches the recorded stamp (meaning source hasn't changed).
:: ============================================================
:cc
set "SRC=%~1"
set "OBJ=%~2"
set "STAMP=%OBJ%.stamp"

:: Get current source file size
for %%F in ("%SRC%") do set "CUR_SIZE=%%~zF"

:: If .o exists, check stamp
if exist "%OBJ%" (
    if exist "%STAMP%" (
        set /p PREV_SIZE=<"%STAMP%"
        if "!PREV_SIZE!"=="!CUR_SIZE!" (
            set /a SKIPPED_COUNT+=1
            goto :eof
        )
    )
)

:: Compile
echo   [CC] %SRC%
%CXX% %CXXFLAGS% -c "%SRC%" -o "%OBJ%" 2>&1
if !errorlevel! neq 0 (
    echo   [FAIL] %SRC%
    set /a BUILD_ERRORS+=1
) else (
    set /a COMPILED_COUNT+=1
    :: Write stamp (source file size)
    echo !CUR_SIZE!> "%STAMP%"
)
goto :eof
