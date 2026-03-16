@echo off
echo [S.A.G.A. CLEANUP] Removing web app and legacy artifacts...

:: 1. Remove old web folders
if exist "taleweaver_vtt" rd /s /q "taleweaver_vtt"
if exist "saga_vtt_client" rd /s /q "saga_vtt_client"

:: 2. Clean build and bin directories
if exist "build" rd /s /q "build"
if exist "bin" mkdir bin

:: 3. Clean Python caches
for /d /r . %%d in (__pycache__) do @if exist "%%d" rd /s /q "%%d"

echo [C++ ARCHITECT BUILD] Compiling God Engine...
:: Use your existing build.bat logic but target strictly App_Architect
call build.bat

echo [PYTHON SETUP] Verifying S.A.G.A. Organs...
pip install arcade imgui-bundle pydantic chromadb

echo Done. Run 'python main.py' to launch the Engine.