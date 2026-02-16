@echo off
TITLE SAGA Oracle Launcher
echo [LAUNCHER] Starting Unified Oracle System...
echo [LAUNCHER] This will open the Server (Python) and the Frontend (Vite/Node).

:: 1. Clear Port 8000 (SAGA Brain) to prevent "Only one usage of each socket address" error
echo [LAUNCHER] Cleaning up any stale server processes on port 8000...
for /f "tokens=5" %%a in ('netstat -aon ^| findstr :8000') do (
    taskkill /F /PID %%a >nul 2>&1
)

:: 2. Start Python Server in background
echo [LAUNCHER] Starting SAGA Brain (scripts/server.py)...
start "SAGA Backend" cmd /k "python scripts/server.py"

:: 3. Start Internal Frontend
echo [LAUNCHER] Navigating to taleweaver_vtt...
cd taleweaver_vtt

if not exist node_modules (
    echo [LAUNCHER] Installing dependencies (this may take a minute)...
    call npm install
)

echo [LAUNCHER] Starting Frontend...
npm run dev

pause
