@echo off
TITLE SAGA Oracle Launcher
echo [LAUNCHER] Starting Unified Oracle System...
echo [LAUNCHER] This will open the Server (Python) and the Frontend (Vite/Node).

:: 1. Start Python Server in background
echo [LAUNCHER] Starting SAGA Brain (scripts/server.py)...
start "SAGA Backend" cmd /k "python scripts/server.py"

:: 2. Start Internal Frontend
echo [LAUNCHER] Navigating to taleweaver_vtt...
cd taleweaver_vtt

if not exist node_modules (
    echo [LAUNCHER] Installing dependencies (this may take a minute)...
    call npm install
)

echo [LAUNCHER] Starting Frontend...
npm run dev

pause
