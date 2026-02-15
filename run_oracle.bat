@echo off
TITLE SAYA Oracle Launcher

echo [LAUNCHER] Starting Unified Oracle System...
echo [LAUNCHER] This will open the Server (Python) and the Frontend (Vite/Node).

:: 1. Start Python Server in background
start "SAYA Backend" cmd /k "cd scripts && python unified_server.py"

:: 2. Start Frontend
echo [LAUNCHER] Navigating to BRQSE Oracle VTT...
cd ..\BRQSE\oracle-vtt

echo [LAUNCHER] Installing dependencies (first run only, usually fast)...
call npm install

echo [LAUNCHER] Starting Frontend...
npm run dev

pause
